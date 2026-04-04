from flask import Flask, render_template, jsonify
import serial
import threading
import time

app = Flask(__name__)

# Variáveis globais para armazenar os dados lidos
dados_atuais = {
    "corrente": 0.0,
    "potencia": 0.0,
    "total_kwh": 0.0
}

def leitura_arduino():
    global dados_atuais
    # AJUSTE A PORTA SERIAL AQUI
    try:
        ser = serial.Serial('COM3', 9600, timeout=1)
        while True:
            if ser.in_waiting > 0:
                linha = ser.readline().decode('utf-8').strip()
                if "Corrente_A:" in linha:
                    corrente = float(linha.split(":")[1])
                    potencia = corrente * 127.0 # Assumindo 127V
                    # Simulação simples de acúmulo para o exemplo
                    dados_atuais["corrente"] = corrente
                    dados_atuais["potencia"] = potencia
                    dados_atuais["total_kwh"] += (potencia / 3600000) # Incremento por segundo
            time.sleep(0.1)
    except:
        print("Erro na Serial. Verifique a conexão.")

# Rota principal que carrega a página
@app.route('/')
def index():
    return render_template('index.html')

# Rota que o JavaScript vai consultar para pegar os números novos
@app.route('/dados')
def dados():
    return jsonify(dados_atuais)

if __name__ == '__main__':
    # Inicia a leitura do Arduino em segundo plano
    threading.Thread(target=leitura_arduino, daemon=True).start()
    app.run(debug=True, port=5000, use_reloader=False)