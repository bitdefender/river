(function(ns) {
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

	ns.MemoryView = function(baseAddress, size, model) {
		this.baseAddress = baseAddress;
		this.size = size;
		this.model = model;
		
		this.stringAddress = PrintHex(baseAddress);
		this.container = document.createElement('div');

		// <div id="breakpoints_window" caption="Breakpoints" class="breakpoints-window"></div>
		this.container.id = "memory_view_" + this.stringAddress + "_window";
		this.container.className = "memory-view-window";

		this.width = 16;

		var tmp = document.createAttribute("caption");
		tmp.value = "Memory @" + this.stringAddress;
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
                    "title" : "Hexadecimal", 
                    "data" : "hexString"
                }, { 
                    "title" : "ASCII", 
                    "data" : "ascii"
                }
            ]
        });

		this.memoryGrid = tbl.dataTable();
	};

	/*ns.MemoryView.prototype.SetData = function (newData) {
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

	ns.MemoryView.prototype.Update = function () {
		var newTable = [];

		for (var i = 0; i < this.model.memory.length; i += this.width) {
			var newRow = {
				address: PrintHex(this.baseAddress + i),
				hexString: "",
				ascii: ""
			};

			for (var j = 0; j < this.width; ++j) {
				newRow.hexString += PrintHexByte(this.model.memory[i + j]) + " ";
				newRow.ascii += PrintAsciiByte(this.model.memory[i + j]);
			}

			newTable.push(newRow);
		}

		this.data = newTable;

        this.memoryGrid.fnClearTable();
        if (0 < this.data.length) {
            this.memoryGrid.fnAddData(this.data);
        } 	
	};

})(window);