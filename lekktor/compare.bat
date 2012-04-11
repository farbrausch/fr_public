stripasm "..\werkkzeug3\player release\%1.asm" testa.txt
stripasm "..\werkkzeug3_lekktor\player release\%1.asm" testb.txt


windiff testa.txt testb.txt
