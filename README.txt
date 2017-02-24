This is a bit of a git mish-mash. The original project was in
EventFramework and had a .cpp extension.  In order to compile with the
Arduino tool chain, you have to use a .ino extension. But the Arduino
tool chain will happily include any .cpp and .h files in the directory
that holds the .ino file. And it throws up in its mouth on directories
that include a '-' so to avoid both problems, the cpp file is sym linked
to the ino file (e.g. ../TEV/TEV.ino. Probably some naming policies in
the Arduino dev kit also working there.
