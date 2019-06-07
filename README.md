# bot-trader
Bot para grupo telegram que realiza operações no metatrader 4 e 5

## Instruções:
### Automático:
- Execute o make.bat como administrador.

### Manualmente:
Lista de dependências: https://github.com/darwinex/DarwinexLabs/tree/master/tools/dwx_zeromq_connector#example-usage

1. Instale o python3 e o pip mais atuais.
2. instale o pyzmq (pip install pyzmq).
3. instale o MetaTrader5 (pip install MetaTrader5).
4. instale o telepot (pip install telepot).
5. instale o libzmq (ZeroMQ 4.0.4)

### Steps:
1. Download and unzip mql-zmq-master.zip (by GitHub author @dingmaotu)
2. Copy the contents of mql-zmq-master/Include/Mql and mql-zmq-master/Include/Zmq into your MetaTrader installation's MQL4/Include directory as-is. Your MQL4/Include directory should now have two additional folders "Mql" and "Zmq".
3. Copy libsodium.dll and libzmq.dll from mql-zmq-master/Library/MT4 to your MetaTrader installation's MQL4/Libraries directory.
4. Download DWX_ZeroMQ_Server_vX.Y.Z_RCx.mq4 and place it inside your MetaTrader installation's MQL4/Experts directory.
5. Finally, download vX.Y.Z / python / api / DWX_ZeroMQ_Connector_vX_Y_Z_RCx.py.