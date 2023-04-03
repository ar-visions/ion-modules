# Precompiling the module
clang++ -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX13.1.sdk -std=c++20 interface_part.cppm --precompile -o ./obj/m-interface_part.pcm
clang++ -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX13.1.sdk -std=c++20 part.cppm      --precompile -fprebuilt-module-path=./obj -o ./obj/m-impl_part.pcm
clang++ -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX13.1.sdk -std=c++20 m.cppm         --precompile -fprebuilt-module-path=. -o ./obj/m.pcm
clang++ -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX13.1.sdk -std=c++20 m.cpp          -fmodule-file=m=m.pcm -c -o ./obj/m.o

# Compiling the user
clang++ -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX13.1.sdk -std=c++20 user.cpp -fprebuilt-module-path=. -c -o ./obj/user.o

# Compiling the module and linking it together
clang++ -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX13.1.sdk -std=c++20 m-interface_part.pcm -c -o m-interface_part.o
clang++ -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX13.1.sdk -std=c++20 ./obj/m-part.pcm      -c -o m-part.o
clang++ -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX13.1.sdk -std=c++20 ./obj/m.pcm -c -o m.o

clang++ -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX13.1.sdk user.o     m-interface_part.o  m-part.o m.o impl.o -o cppm-hello
