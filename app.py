from flask import Flask, render_template, jsonify
import serial
import threading
import time

app = Flask(__name__)

# Memória compartilhada
estado_sistema = {
    "conectado": False,
    "dados_atuais": {"corrente": 0.0, "potencia": 0.0, "total": 0.0},
    "historico": [0.0] * 168
}

def monitor_serial():
    global estado_sistema
    porta = 'COM3' # Ajuste conforme seu sistema
    
    while True:
        try:
            with serial.Serial(porta, 9600, timeout=1) as ser:
                estado_sistema["conectado"] = True
                print(f"Conectado em {porta}")
                
                while True:
                    if ser.in_waiting > 0:
                        linha = ser.readline().decode('utf-8', errors='ignore').strip()
                        partes = linha.split('|')
                        
                        if partes[0] == "REAL_TIME" and len(partes) == 4:
                            estado_sistema["dados_atuais"] = {
                                "corrente": float(partes[1]),
                                "potencia": float(partes[2]),
                                "total": float(partes[3])
                            }
                        
                        elif partes[0] == "HIST" and len(partes) == 3:
                            idx = int(partes[1])
                            valor = float(partes[2])
                            if idx < 168:
                                estado_sistema["historico"][idx] = valor

                    time.sleep(0.1)
        except (serial.SerialException, FileNotFoundError):
            estado_sistema["conectado"] = False
            print("Arduino desconectado. Tentando reconectar...")
            time.sleep(2)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/dados')
def get_dados():
    # Processa histórico para dados diários (soma simples para o gráfico)
    # Pega o último registro de cada bloco de 24h para representar o acumulado do dia
    historico_diario = []
    for i in range(23, 168, 24):
        historico_diario.append(round(estado_sistema["historico"][i], 2))
        
    return jsonify({
        "status": "Conectado" if estado_sistema["conectado"] else "Arduino desconectado",
        "real_time": estado_sistema["dados_atuais"],
        "graph_data": historico_diario
    })

if __name__ == '__main__':
    threading.Thread(target=monitor_serial, daemon=True).start()
    app.run(debug=False, port=5000)