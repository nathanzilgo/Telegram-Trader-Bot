# coding:utf-8
# bot token: 665972859:AAH1u3Wio7GEUko8R_CPAD6gy7nINHlxPJ0

from lib.dwx_zeromq_connector_master.version2.python.api.DWX_ZeroMQ_Connector_v2_0_1_RC8.py import *
from MetaTrader5 import *
from time import sleep
from datetime import datetime
import telepot
import pandas

greetings = """
#############################################################
#   Trader 1.0 by Nathan Fernandes. @nathanzilgo on github  #
#           This is the main exec file                      #
#############################################################
"""

def botInit(botId = '841455460:AAETi2oNThKRxHUxvOTj5iRZZ2mlaItEd8s'):
    bot = telepot.Bot(botId)
    return bot

def getOrder(bot):
    texto = bot.getUpdates(['text'])
    getToken(texto)
    getSignal(texto)
    getProfit(texto)

    if 'sell' in texto:
        sell()
    elif 'buy' in texto:
        buy()

def buy(token = '', profit = '', signal = ''):
    token = token + micro
    Connector()._DWX_MTX_SEND_COMMAND_('OPEN', 0, token, profit, 50, 50, "Python to MT",
    lote, 123456, signal)

    Connector()._DWX_MTX_CLOSE_ALL_TRADES_()

def sell(token, profit, signal):
    token = token + micro
    Connector()._DWX_MTX_SEND_COMMAND_('OPEN', 1, token, profit, 50, 50, "Python to MT",
    lote, 123456, signal)

    Connector()._DWX_MTX_CLOSE_ALL_TRADES_()

def getSignal(text):
    first_line = text.split("/n")[1]
    return first_line.split()[-1]

def getToken(text):
    second_line = text.split("/n")[2]
    return second_line[1]

def getProfit(text):
    third_line = text.split("/n")[-1]
    return int(third_line[-1])

def runtime(lote, micro):
    while True:
        try:
            print("Testes:")
            MT5Initialize("")
            MT5WaitForTerminal()
            
            print(MT5TerminalInfo())
            print("Versão atual do MT5: ", MT5Version())
            print("Inicializando o bot...")
            bot = botInit()

            bot.message_loop(getOrder(bot)) # executa sempre que uma mensagem é chamada
            
        except MT5WaitForTerminal()==False:
            print("Erro em tempo de excecução. Veririfique o PATH do MT5 e sua conexão.")
            print("Tentando reconectar...")
            runtime(lote, micro)
            break

print(greetings)

lote = int(input("Forneça o lote a ser comprado: "))
micro = input("Conta MICRO ou não? (S/N) ")
user = input("Informe o id de usuário: ")

if micro.upper() == 'N': micro = ''
else: micro = 'Micro'

connector = Connector(user)

runtime(lote, micro)