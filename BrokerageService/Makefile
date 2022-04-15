CC = gcc
CFLAG = -g -Wall

key:keymanage
keymanage: KeyManage.c DieWithError.c
	$(CC) $(CFLAGS) -o keymanage KeyManage.c DieWithError.c sma.c

broker:Broker
SRCSb = KeyGen.c DieWithError.c Enc.c Dec.c StructToInt.c IntToStruct.c SetKey.c GetKey.c HandleTCPBroker.c sma.c
MAINb = Broker
$(MAINb): $(MAINb).c $(SRCS)
	$(CC) $(CFLAGS) -o $(MAINb) $(MAINb).c $(SRCSb)

client:Client
SRCSc = DieWithError.c KeyGen.c Enc.c Dec.c StructToInt.c IntToStruct.c SetKey.c GetKey.c sma.c
MAINc = Client
$(MAINc): $(MAINc).c $(SRCSc)
	$(CC) $(CFLAGS) -o $(MAINc) $(MAINc).c $(SRCSc)


ALL: 
	make key
	make broker
	make client

clear:
	rm -rf *.o *~ Broker Client keymanage
