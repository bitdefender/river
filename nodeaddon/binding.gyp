{
    "targets": [
        {
            "target_name": "executionwrapper",
            "sources": [ "executionwrapper.cpp" ],
            "include_dirs": [
            	"../Execution",
                "<!(node -e \"require('nan')\")"
            ],
            "libraries": [
            	"-lD:/wrk/riverhg/Debug/Execution.lib",
            ]
        }
    ]
}