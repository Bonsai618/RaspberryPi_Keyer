  include <P18CXXX.INC>
        
UARTDATA        UDATA
uartdata        RES     1
BitCount        RES     2
Msg1Ready       RES     2
Buffer1         RES     256
nQueue          RES     2
nQueue2         RES     2
QHead           RES     2
QTail           RES     2
QReset          RES     2
Semaphore       RES     2
QBuff		    RES     256
QBuff2          RES     40



    	GLOBAL	uartdata, BitCount, QReset, Semaphore
        GLOBAL  Msg1Ready, Buffer1, QBuff	, nQueue, QHead, QTail, QBuff2, nQueue2;
        END
        

