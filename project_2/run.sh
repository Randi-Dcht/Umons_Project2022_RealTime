gcc -pthread parser.c logger.c stock.c -o stock.bin -lrt
gcc -pthread parser.c logger.c maker.c -o maker.bin -lrt
gcc -pthread parser.c logger.c customer.c -o customer.bin -lrt

touch startMe

./maker.bin $1 True &
./customer.bin $1 &
./stock.bin $1

rm *.bin startMe mmFacto* mCusto*