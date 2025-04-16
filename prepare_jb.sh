(
echo "Assigning PATHS";
export VITASDK=/usr/local/vitasdk;
export PATH=$VITASDK/bin:$PATH # add vitasdk tool to $PATH;

echo "Compiling the bootstrap";
cd bootstrap_lite/;
touch *.c;
make;
xxd -i bootstrap.self > ../payload/bootstrap.h;
rm *.o && rm *.elf && rm *.velf && rm *.self;
echo "Compiling the payload";
cd ../payload;
touch *.c;
make;
mv payload.bin ../go/payload.bin
rm *.o && rm *.elf && rm bootstrap.h;
echo "";
echo "DONE!";
echo "";
)