#coding:utf-8
#bot token: 665972859:AAH1u3Wio7GEUko8R_CPAD6gy7nINHlxPJ0

from MetaTrader5 import *
from time import sleep
from datetime import datetime

def runtime():

    while True:
        try:
            MT5Initialize()
            MT5WaitForTerminal()

            print("Testes:")
            print(MT5TerminalInfo())
            print("Versão atual do MT5: ", MT5Version())
            
        

        except EOFError:
            print("Erro de execução no bot!")

