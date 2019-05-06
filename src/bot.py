#coding:utf-8
#bot token: 665972859:AAH1u3Wio7GEUko8R_CPAD6gy7nINHlxPJ0

from MetaTrader5 import *
from time import sleep
from datetime import datetime
import telepot

tokens = []
class trader():
    
    def __init__(self, lote_user, chatId):
        self.lote = lote_user # Define o lote de compra da sessão
        self.name = "665972859:AAH1u3Wio7GEUko8R_CPAD6gy7nINHlxPJ0" 
        self.bot = telepot.Bot(name) # Seleciona e inicializa o bot
        self.runtime()  # Inicia tudo
        self.chatId = chatId # ID para o bot enviar e receber mensagens

    def getOrder():
        texto = self.bot.getUpdates(['text'])

        if 'sell' in texto:
            return 'sell'
        else if 'buy' in texto:
            return 'buy'

    def buy(signal, currency, value):
        self.bot.sendMessage("Signal no ", signal)

    def sell(signal, currency, value, profit):
        self.bot.sendMessage("Signal no ", signal)

    def takeProfit(value):
    
    # INICIALIZA O BOT E O MQL 5
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

