make clean
rm -f cmake_install.cmake
rm -f CMakeCache.txt
rm -f Makefile
rm -f -rf CMakeFiles

for f in $(find ./ -name 'Makefile' -or -name 'CMakeFiles' -or -name 'cmake_install.cmake' -or -name 'cmake_install.cmake' -or -name 'CMakeCache.txt'); 
do 
	rm -f -rf $f; 
done

