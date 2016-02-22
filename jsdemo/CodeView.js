(function(ns) {
    var disasm = new capstone.Cs(capstone.ARCH_X86, capstone.MODE_32);

	function PrintHex(value) {
        return ("00000000" + (value >>> 0).toString(16)).substr(-8);
    }

    function PrintHexByte(value) {
    	return ("00" + (value & 0xFF).toString(16)).substr(-2);	
    }

    function PrintAsciiByte(value) {
    	if ((value < 32) || (value >= 127)) {
    		return ".";
    	}
    	return String.fromCharCode(value);
    }

    ns.IsActualCode = function(buffer) {
        try {
            var newInstr = disasm.disasm(buffer, 0);
        } catch (ex) {
            return false;
        }
        return true;
    }

	ns.CodeView = function(baseAddress, size, model) {
		this.baseAddress = baseAddress;
		this.size = size;
        this.model = model;

		this.stringAddress = PrintHex(baseAddress);
		this.container = document.createElement('div');

		// <div id="breakpoints_window" caption="Breakpoints" class="breakpoints-window"></div>
		this.container.id = "code_view_" + this.stringAddress + "_window";
		this.container.className = "code-view-window";

		this.width = 16;

		var tmp = document.createAttribute("caption");
		tmp.value = "Code @" + this.stringAddress;
		this.container.setAttributeNode(tmp);

        var _this = this;
        this.container.onresize = function() {
            _this.memoryGrid.fnAdjustColumnSizing(true);
            var st = _this.memoryGrid.fnSettings();

            st.oScroll.sY = (_this.container.clientHeight - 24) + "px";
            _this.memoryGrid.fnDraw();
        };

		this.content = document.createElement('table');
		this.content.className = "display compact";
        this.content.style.cssText = "width:100%; height:100%";
		//this.content.id = "memory_view_" + this.stringAddress + "_content";

		this.container.appendChild(this.content);
		document.body.appendChild(this.container);

		this.data = [];
		this.memory = [];

        var tbl = $(this.content);
        tbl.DataTable({
            "paging" : false,
            "scrollY" : "calc(100% - 24px)",
            //"dom" : '<"fixed_height"t>',
            "dom" : "t",
            /*"responsive" : {
                "details" : false
            },*/
            "deferRender" : true,
            "data" : [],
            "language": {
                "emptyTable": "",
                "infoEmpty": "",
                "zeroRecords": ""
            },
            "columns" : [
                { 
                    "title" : "Address", 
                    "data" : "address"
                }, { 
                    "title" : "Code", 
                    "data" : "code"
                }, { 
                    "title" : "Hexadecimal", 
                    "data" : "hexString"
                }
            ]
        });

		/*this.columns = [
            {
                id: "address", 
                name: "Address", 
                field: "address", 
                sortable: true,
                width: 80,
                minWidth: 80
            }, {
                id: "instr", 
                name: "code", 
                field: "code", 
                sortable: true,
                width: 360,
                minWidth: 360
            }, {
                id: "hex", 
                name: "Hexadecimal", 
                field: "hexString", 
                sortable: true,
                width: 150,
                minWidth: 150
            }
        ];

        this.options = {
            enableCellNavigation: true,
            enableColumnReorder: true,
            fullWidthRows: true,
            forceFitColumns: true,
            headerRowHeight: 22,
            rowHeight: 22,
            syncColumnCellResize: false
        };*/

        this.memoryGrid = tbl.dataTable(); //new Slick.Grid(this.content, this.data, this.columns, this.options);
        //this.memoryGrid.setSelectionModel(new Slick.RowSelectionModel());
	};

	/*ns.CodeView.prototype.SetData = function (newData) {
		this.memory = newData;

		var newTable = [];
		for (var i = 0; i < this.memory.length; i += this.width) {
			var newRow = {
				address: PrintHex(this.baseAddress + i),
				hexString: "",
				ascii: ""
			};

			for (var j = 0; j < this.width; ++j) {
				newRow.hexString += PrintHexByte(this.memory[i + j]) + " ";
				newRow.ascii += PrintAsciiByte(this.memory[i + j]);
			}

			newTable.push(newRow);
		}

		this.data = newTable;

		this.memoryGrid.setData(this.data, false);
        this.memoryGrid.render();
	};*/

    ns.CodeView.prototype.Update = function () {
        var newTable = [];

        try {
            var newInstr = disasm.disasm(this.model.memory, this.baseAddress);

            for (var i = 0; i < newInstr.length; ++i) {
                var newRow = {
                    address: PrintHex(newInstr[i].address),
                    hexString: "",
                    code: newInstr[i].mnemonic + " " + newInstr[i].op_str
                };

                for (var j = 0; j < newInstr[i].bytes.length; ++j) {
                    newRow.hexString += PrintHexByte(newInstr[i].bytes[j]) + " ";
                }

                newTable.push(newRow);
            }

            this.data = newTable;
        } catch (ex) {
            console.log(ex);
        }

        this.memoryGrid.fnClearTable();
        if (0 < this.data.length) {
            this.memoryGrid.fnAddData(this.data);
        } 
    };

})(window);