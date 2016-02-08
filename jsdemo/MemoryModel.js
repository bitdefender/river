(function(ns) {
	ns.MemoryModel = function(controller, address, size) {
		this.controller = controller;
		this.address = address;
		this.size = size;
		this.memory = controller.ReadProcessMemory(this.address, this.size);

		this.views = [];
	};

	ns.MemoryModel.prototype.Notify = function() {
		for (var i = 0; i < this.views.length; ++i) {
			this.views.Update();
		}
	};

	ns.MemoryModel.prototype.Update = function() {
		var newMem = this.controller.ReadProcessMemory(this.address, this.size);

		var ch = false;
		if (this.memory.length == newMem.length) {
			for (var j = 0; j < this.memory.length; ++j) {
				if (this.memory[j] != newMem[j]) {
					ch = true;
					break;
				}
			}
		}

		if ((this.memory.length != newMem.length) || (ch)) {
			this.memory = newMem;
			this.Notify();
		}
	};

	ns.MemoryModel.prototype.AddView = function(newView) {
		this.views.push(newView);
	};

	ns.MemoryModel.prototype.RemoveView = function(view) {
		var idx = this.views.indexOf(view);
		if (idx > -1) {
 			this.views.splice(idx, 1);
		}

		return (0 == this.views.length);
	}

})(window);