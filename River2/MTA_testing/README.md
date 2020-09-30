In order to use the simulation, one must have libFuzzer (https://llvm.org/docs/LibFuzzer.html) installed.

We used Ubuntu 16.04 for our simulation. 
Steps needed to run the tests:
- decompress archive: 7z x multi-tenancy-testing.7z
- start a terminal for the web application: cd MTA/app && . reset.sh
- start a terminal for testing: cd MTA/sim && . activate.sh && python main.py

If all works well, the users will start interacting with the web application and after it finishes there will be a new folder in MTA/sim with the simulation results.
