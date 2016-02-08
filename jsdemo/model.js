var demoModel = (function() {

	var exec = require("exec");
	var controller = new exec.ExecutionWrapper();

	var statusValues = {
		IDLE : 0x00,
		GO_TO_END: 0x01,
		GO_TO_BEGINNING: 0x02
	};

	var status = statusValues.IDLE;

	function GetEnvironment() {
		var ret = "";

		for (var i in process.env) {
			ret += i + "=\"" + process.env[i] + "\"\n";
		}

		return ret;
	}

	var availableStates = {
		EXECUTION_NEW : 0x00,
		EXECUTION_INITIALIZED : 0x01,
		EXECUTION_SUSPENDED_AT_START : 0x02,
		EXECUTION_RUNNING : 0x03,
		EXECUTION_SUSPENDED : 0x04,
		EXECUTION_SUSPENDED_AT_TERMINATION : 0x05,
		EXECUTION_TERMINATED : 0x06,
		EXECUTION_ERR : 0xFF
	};

	var state = availableStates.EXECUTION_NEW;
	var currentIp;

	var exeName = "";
	var cmdLine = "";
	var envVariables = GetEnvironment();

	var cRegisters = {
		"eax" : {"value" : "00000000", "class" : "reg-disabled"},
		"ecx" : {"value" : "00000000", "class" : "reg-disabled"},
		"edx" : {"value" : "00000000", "class" : "reg-disabled"},
		"ebx" : {"value" : "00000000", "class" : "reg-disabled"},

		"esp" : {"value" : "00000000", "class" : "reg-disabled"},
		"ebp" : {"value" : "00000000", "class" : "reg-disabled"},
		"esi" : {"value" : "00000000", "class" : "reg-disabled"},
		"edi" : {"value" : "00000000", "class" : "reg-disabled"},

		"eflags" : {"value" : "00000000", "class" : "reg-disabled"},

		"eip" : {"value" : "00000000", "class" : "reg-disabled", "number" : 0}
	};

	var memoryMap = [];
	var modules = [];
	var breakpoints = [];
	var waypoints = [];

	var executedBlocks = 0;

	var memoryViews = [];

	function FindMemoryView(baseAddress, size) {
		for (var i = 0; i < memoryViews.length; ++i) {
			if ((memoryViews[i].baseAddress == baseAddress) && (memoryViews[i].size == size)) {
				return memoryViews[i];
			}
		}
		return false;
	}

	var memoryModels = [];

	var toolVisibility = {
		modules: true,
		address_space: true,
		registers: true,

		breakpoints: true,
		waypoints: true,

		river_stack: true,
		track_stack: true,
		execution_log: true,

		inspector: true
	};

	var eventListeners = {};

	var SetState = function(newState) {
		state = newState;
		model.trigger("StateChanged");
	}

	var CheckState = function() {
		var s = controller.GetState();

		if (s != state) {
			SetState(s);

			if ((s == availableStates.EXECUTION_SUSPENDED) || 
				(s == availableStates.EXECUTION_SUSPENDED_AT_TERMINATION) ||
				(s == availableStates.EXECUTION_SUSPENDED_AT_START)) {
					model.UpdateRegistersModel();
					model.UpdateMemoryMapModel();
					model.UpdateMemoryModel();
					model.UpdateModulesModel();
					model.UpdateEIP(currentIp);
			}

			if (s == availableStates.EXECUTION_TERMINATED) {
				memoryMap.length = 0;
				modules.length = 0;
				waypoints.length = 0;

				cRegisters.eip.number = 0;

				for (var i in cRegisters) {
					cRegisters[i]["value"] = "00000000";
					cRegisters[i]["class"] = "reg-disabled";
				}

				model.trigger("CurrentIPChanged");
				model.trigger("RegistersChanged");
				model.trigger("MemoryMapChanged");
				model.trigger("ModulesChanged");
				model.trigger("WaypointsChanged");
				model.UpdateBreakpoints();
			}
		}
	}

	var BindBreakpoint = function(bkp, lookup) {
		if (bkp.state == "bound") {
			return true;
		}

		if (!("module" in bkp)) {
			bkp.base = 0;
			bkp.state = "bound";
			return true;
		}

		if ("undefined" === typeof lookup) {
			lookup = model.GetModulesLookup();
		}

		var extensions = ["", ".dll", ".exe"];

        for (var i in extensions) {
            var nMod = bkp.module + extensions[i];
            if (nMod in lookup) {
                bkp.base = lookup[nMod];
                bkp.state = "bound";
                return true;
            }
        }

        return false;
	};

	var BreakpointHit = function(eip) {
		var breakpoints = model.GetBreakpoints();
		for (var i in breakpoints) {
			if ("bound" == breakpoints[i].state) {
				if (eip == breakpoints[i].base + breakpoints[i].offset) {
					return true;
				}
			}
		}

		return false;
	};

	var WaypointHit = function(blocks) {
		var waypoints = model.GetWaypoints();

		if (0 == waypoints.length) {
			return false;
		}

		var lastWpCtr = waypoints[waypoints.length - 1].number;

		return lastWpCtr == blocks;
	};

	controller.SetTerminationNotification(function() {
		CheckState();
	});

	controller.SetExecutionBeginNotification(function(eip) {
		//model.UpdateEIP(eip);
		currentIp = eip;
		if (statusValues.GO_TO_BEGINNING == status) {
			status = statusValues.IDLE;
		}
		CheckState();
	});
	
	controller.SetExecutionControlNotification(function(eip) {
		//model.UpdateEIP(eip);
		currentIp = eip;
		switch (status) {
			case statusValues.IDLE :
				CheckState();
				break;
			case statusValues.GO_TO_BEGINNING :
				if (WaypointHit(executedBlocks)) {
					status = statusValues.IDLE;
					CheckState();
				} else {
					executedBlocks--;
					controller.Control(1);	
				}
				break;
			case statusValues.GO_TO_END :
				if (BreakpointHit(eip)) {
					status = statusValues.IDLE;
					CheckState();
				} else {
					executedBlocks++;
					controller.Control(0);
				}
				break;
		}
	});

	controller.SetExecutionEndNotification(function() {
		if (statusValues.GO_TO_END == status) {
			status = statusValues.IDLE;
		}
		CheckState();
	});


	var model = {
		GetState : function () {
			return state;
		},

		GetExeName : function() {
			return exeName;
		},

		CanChangeExeName : function() {
			return (state == availableStates.EXECUTION_NEW) ||
				(state == availableStates.EXECUTION_INITIALIZED) ||
				(state == availableStates.EXECUTION_TERMINATED);
		},

		SetExeName : function(en) {
			if (model.CanChangeExeName()) {
				exeName = en;
				controller.SetPath(exeName);
				model.trigger("ExeNameChanged");
				
				CheckState();
			}
		},

		GetCmdLine : function() {
			return cmdLine;
		},

		CanChangeCmdLine : function() {
			return 
				(state == availableStates.EXECUTION_NEW) ||
				(state == availableStates.EXECUTION_INITIALIZED) ||
				(state == availableStates.EXECUTION_TERMINATED);
		},

		SetCmdLine : function(cl) {
			if (model.CanChangeCmdLine()) {
				cmdLine = cl;
				model.trigger("CmdLineChanged");
				CheckState();
			}
		},

		GetEnvironmentVariables : function() {
			return envVariables;
		},

		CanChangeEnvironmentVariables : function() {
			return 
				(state == availableStates.EXECUTION_NEW) ||
				(state == availableStates.EXECUTION_INITIALIZED) ||
				(state == availableStates.EXECUTION_TERMINATED);
		},

		SetEnvironmentVariables : function(ev) {
			if (model.CanChangeEnvironmentVariables()) {
				envVariables = ev;
				model.trigger("EnvironmentVariablesChanged");
				CheckState();
			}
		},

		GetVisibleTools : function() {
			var ret = [];
			for (var p in toolVisibility) {
				if (toolVisibility[p]) {
					ret.push(p);
				}
			}

			return ret;
		},

		GetToolVisibility : function(panel) {
			if (panel in toolVisibility) {
				return toolVisibility[panel];
			}
			return false;
		},

		SetToolVisibility : function(panel, vis) {
			if (panel in toolVisibility) {
				if (toolVisibility[panel] != vis) {
					toolVisibility[panel] = vis;
					model.trigger("ToolVisibilityChanged");
				}
			}
		}, 

		GetCurrentIp : function() {
			return cRegisters.eip.number;
		},

		GetRegisters : function() {
			return cRegisters;
		},

		GetMemoryMap : function() {
			return memoryMap;
		},

		GetModules : function() {
			return modules;
		},

		GetModulesLookup : function() {
			var ret = {};
			for (var i in modules) {
				ret[modules[i].name] = modules[i].moduleBase;
			}
			return ret;
		},

		GetBreakpoints : function() {
			return breakpoints;
		},

		GetWaypoints : function() {
			return waypoints;
		},

		GetMemoryModel : function(baseAddress, size) {
			for (var i = 0; i < memoryModels.length; ++i) {
				if ((memoryModels[i].address == baseAddress) && (memoryModels[i].size == size)) {
					return memoryModels[i];
				}
			}

			var ret = new MemoryModel(controller, baseAddress, size);
			memoryModels.push(ret);
			return ret;
		},

		CreateMemoryView : function(baseAddress, size) {
			if (FindMemoryView(baseAddress, size)) {
				return undefined;
			}

			var mdl = this.GetMemoryModel(baseAddress, size);

			var rt = new MemoryView(baseAddress, size, mdl);
			memoryViews.push(rt);

			mdl.AddView(rt);
			rt.Update();
			return rt;
		},

		CreateCodeView : function(baseAddress, size) {
			if (FindMemoryView(baseAddress, size)) {
				return undefined;
			}

			var mdl = this.GetMemoryModel(baseAddress, size);
			var rt;
			if (IsActualCode(mdl.memory)) {
				rt = new CodeView(baseAddress, size, mdl);
			} else {
				rt = new MemoryView(baseAddress, size, mdl);
			}

			
			memoryViews.push(rt);

			mdl.AddView(rt);
			rt.Update();
			return rt;
		},

		CreateBreakpoint : function(bkp) {
			var ret = {
				str : bkp,
				state : "unbound"
			};

			var sep = bkp.indexOf("+");
            if (-1 != sep) {
                var modName = bkp.substring(0, sep).toLowerCase();
                
                if (null == modName.match(/^[a-z_\\-\\.]+$/)) {
                	return false;
                }

                ret.module = modName;
            }

            var addr = bkp.substring(sep + 1);
            var sAddr = parseInt(addr, 16);
            if (Number.isNaN(sAddr)) {
                return false;
            } 

            ret.offset = sAddr;

            BindBreakpoint(ret);

            breakpoints.push(ret);
            this.trigger("BreakpointsChanged");

            return true;
		},

		CreateWaypoint : function() {
			waypoints.push({
				number : executedBlocks,
				address : cRegisters.eip.number
			});

			this.trigger("WaypointsChanged");
		},

		Create : function() {
			controller.Execute();
			executedBlocks = 0;
		},

		Control : function(value) {
			SetState(availableStates.EXECUTION_RUNNING);

			if (0 == value) {
				executedBlocks++;
			} else if (1 == value) {
				executedBlocks--; 

				var changed = false;
				while ((0 < waypoints.length) && (executedBlocks < waypoints[waypoints.length - 1].number)) {
					changed = true;
					waypoints.pop();
				}

				if (changed) {
					this.trigger("WaypointsChanged");
				}
			}
			
			controller.Control(value);
		},

		GoToBegin : function() {
			status = statusValues.GO_TO_BEGINNING;
			this.Control(1);
		},

		GoToEnd : function() {
			status = statusValues.GO_TO_END;
			this.Control(0);
		},

		UpdateEIP : function(eip) {
			var changed = false;

			var value = ("00000000" + (eip >>> 0).toString(16)).substr(-8);

			cRegisters.eip.number = eip;
			if (cRegisters.eip.value != value) {
				cRegisters.eip["value"] = value;
				cRegisters.eip["class"] = "reg-changed";

				this.trigger("CurrentIPChanged");
			} else {
				if (cRegisters.eip["class"] != "reg-normal") {
					cRegisters.eip["class"] = "reg-normal";
				}
			}
		},

		UpdateRegistersModel : function() {
			var regs = controller.GetCurrentRegisters();
			var nms = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "eflags"];
			var changed = false;

			for (var r in nms) {
				var reg = nms[r];
				var value = ("00000000" + (regs[reg] >>> 0).toString(16)).substr(-8);

				if (cRegisters[reg].value != value) {
					cRegisters[reg]["value"] = value;
					cRegisters[reg]["class"] = "reg-changed";
					changed = true;
				} else {
					if (cRegisters[reg]["class"] != "reg-normal") {
						cRegisters[reg]["class"] = "reg-normal";
						changed = true;
					}
				}
			}

			if (changed) {
				this.trigger("RegistersChanged");
			}
		},

		UpdateMemoryMapModel : function() {
			var newMap = controller.GetProcessVirtualMemory();

			if (newMap.length != memoryMap.length) {
				memoryMap = newMap;

				this.trigger("MemoryMapChanged");
				return;
			}

			for (var i = 0; i < newMap.length; ++i) {
				if ((memoryMap[i].baseAddress != newMap[i].baseAddress) ||
						(memoryMap[i].regionSize != newMap[i].regionSize) ||
						(memoryMap[i].state != newMap[i].state) ||
						(memoryMap[i].protection != newMap[i].protection) ||
						(memoryMap[i].type != newMap[i].type)) {
					memoryMap = newMap;

					this.trigger("MemoryMapChanged");
					return;
				}
			}
		},

		UpdateModulesModel : function() {
			var newModules = controller.GetModules();
			var changed = false;

			if (newModules.length != modules.length) {
				modules = newModules;
				changed = true;
			} else {
				for (var i = 0; i < newModules.length; ++i) {
					if ((modules[i].moduleBase != newModules[i].moduleBase) ||
							(modules[i].size != newModules[i].size) ||
							(modules[i].name != newModules[i].name)) {
						modules = newModules;

						changed = true;
						break;
					}
				}
			}

			if (changed) {
				this.trigger("ModulesChanged");
				this.UpdateBreakpoints();
			}
		},

		UpdateMemoryModel : function() {
			for (var i = 0; i < memoryModels.length; ++i) {
				for (var j = 0; j < memoryMap.length; ++j) {
					if ((memoryMap[j].baseAddress == memoryModels[i].address) || (memoryMap[j].regionSize == memoryModels[i].size)) {
						if (memoryMap[j].protection & 0x04) {
							memoryModels[i].Update();							
						}
					}
				}
			}
		},

		UpdateBreakpoints : function() {
			var modNames = this.GetModulesLookup();
			var changed = false;

			for (var b in breakpoints) {
				switch (breakpoints[b].state) {
					case "bound" : // try to unbind it
						if ("module" in breakpoints[b]) {
							if (!(breakpoints[b].module in modNames)) {
								delete breakpoints[b].base;
								breakpoints[b].state = "unbound";
								changed = true;
							}
						}
						break;
					case "unbound" : // try to bind it
						if (BindBreakpoint(breakpoints[b], modNames)) {
							changed = true;
						}
						break;
				};
			}

			if (changed) {
				this.trigger("BreakpointsChanged");
			}
		},

		trigger : function(eventName) {
			if (eventName in eventListeners) {
				for (var l in eventListeners[eventName]) {
					eventListeners[eventName][l](model);
				}
			}
		},

		on : function(eventName, handler) {
			if (!(eventName in eventListeners)) {
				eventListeners[eventName] = [];
			}

			eventListeners[eventName].push(handler);
		}
	};

	return model;
})();