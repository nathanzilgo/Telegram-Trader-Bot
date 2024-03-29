#coding: utf-8

import socket, numpy as np

class socketserver():
    
    def __init__(self, adress = '', port = 9090):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.adress = adress
        self.port = port
        self.sock.bind((self.adress, self.port))
        self.cummdata = ''

    def recvmsg(self):
        self.sock.listen(1)
        self.conn, self.addr = self.sock.accept()
        print('Connected to', self.addr)
        self.cummdata = ''

        while True:
            data = self.conn.recv(10000)
            self.cummdata += data.decode("utf-8")

            if not data:
                break
            self.conn.send(bytes(calcregr(self.cummdata), "utf-8"))
            return self.cummdata

    def calcregr(self, msg = ''):
        chartdata = np.fromstring(msg, dtype=float, sep= ' ') 
        Y = np.array(chartdata).reshape(-1,1)
        X = np.array(np.arange(len(chartdata))).reshape(-1,1)
            
        lr = LinearRegression()
        lr.fit(X, Y)
        Y_pred = lr.predict(X)
        type(Y_pred)
        P = Y_pred.astype(str).item(-1) + ' ' + Y_pred.astype(str).item(0)
        print(P)
        return str(P)

    def __del__(self): # Destrutor para fim de execução
        self.sock.close()
        