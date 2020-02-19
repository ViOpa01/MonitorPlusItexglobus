#ifndef WALLET__H
#define WALLET__H

int walletRequest(int balanceOrTransfer);
int parseWalletResponse(char* response, int balanceOrTransfer, long long amount);


#endif
