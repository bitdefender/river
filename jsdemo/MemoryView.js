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

		this.content = document.createElement('div');
		this.content.style.cssText = "width:100%; height:100%";
		//this.content.id = "memory_view_" + this.stringAddress + "_content";

		this.container.appendChild(this.content);
		document.body.appendChild(this.container);

		this.data = [];
		
		this.columns = [
            {
                id: "address", 
                name: "Address", 
                field: "address", 
                sortable: true,
                width: 80,
                minWidth: 80
            }, {
                id: "hex", 
                name: "Hexadecimal", 
                field: "hexString", 
                sortable: true,
                width: 374,
                minWidth: 374
            }, {
                id: "ascii", 
                name: "ASCII", 
                field: "ascii", 
                sortable: true,
                width: 136,
                minWidth: 136
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
        };

        this.memoryGrid = new Slick.Grid(this.content, this.data, this.columns, this.options);
        this.memoryGrid.setSelectionModel(new Slick.RowSelectionModel());
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

		this.memoryGrid.setData(this.data, false);
        this.memoryGrid.render();	
	};

})(window);