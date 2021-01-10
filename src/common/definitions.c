
int getProtocol(int checksum){
    switch(checksum){
        case 1:
            return 6;
        case 2:
            return 206;
        default:
            return 6;
    }
}