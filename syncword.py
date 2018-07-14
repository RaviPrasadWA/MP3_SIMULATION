def find_next_syncword(bitbfr):
    #align to byte boundry
    align = bitbfr.get_pos()%8
    if(align != 0):
        bitbfr.read_bits(8-align)

    cnt = 0
    while( bitbfr.bits_left() > 0):
        b = bitbfr.read_bits(4)
        if(b == 0xf):  
            cnt += 1
            if(cnt == 3):
                break
        else:                
            cnt = 0
