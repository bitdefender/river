md %1
md %1\include

md %1\include\CommonCrossPlatform
copy CommonCrossPlatform\Common.h %1\include\CommonCrossPlatform
copy CommonCrossPlatform\BasicTypes.h %1\include\CommonCrossPlatform

md %1\include\Execution
copy Execution\Execution.h %1\include\Execution

md %1\include\revtracer
copy revtracer\common.h %1\include\revtracer
copy revtracer\DebugPrintFlags.h %1\include\revtracer
copy revtracer\environment.h %1\include\revtracer
copy revtracer\revtracer.h %1\include\revtracer
copy revtracer\river.h %1\include\revtracer
copy revtracer\RiverAddress.h %1\include\revtracer

md %1\include\SymbolicEnvironment
copy SymbolicEnvironment\Environment.h %1\include\SymbolicEnvironment
copy SymbolicEnvironment\LargeStack.h %1\include\SymbolicEnvironment
copy SymbolicEnvironment\SymbolicEnvironment.h %1\include\SymbolicEnvironment

md %1\win
md %1\win\lib
copy Debug\Execution.lib %1\win\lib
copy Debug\SymbolicEnvironment.lib %1\win\lib

md %1\win\bin
copy Debug\revtracer.dll %1\win\bin
copy Debug\revtracer.pdb %1\win\bin
copy Debug\revtracer-wrapper.dll %1\win\bin
copy Debug\revtracer-wrapper.pdb %1\win\bin
copy Debug\Execution.dll %1\win\bin
copy Debug\Execution.pdb %1\win\bin
copy Debug\SymbolicEnvironment.dll %1\win\bin
copy Debug\SymbolicEnvironment.pdb %1\win\bin