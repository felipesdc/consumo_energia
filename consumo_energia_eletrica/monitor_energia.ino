#include <EEPROM.h>

// --- CONFIGURAÇÕES DE TESTE ---
#define MODO_TESTE true // Mude para false para a versão final

// --- CONSTANTES E PINOS ---
const int pinSensor = A0;
const float tensaoRede = 127.0;
const unsigned long INTERVALO_SALVAR = 3600000; // 1 hora
const int MAX_REGISTROS = 168; // 7 dias (24h * 7)

// Estrutura da EEPROM
const int ADDR_INDICE = 0;
const int ADDR_DADOS = 2;

float consumoAcumulado = 0.0;
unsigned long ultimoCalculo = 0;
unsigned long ultimoSalvamento = 0;
int indiceAtual = 0;

void setup() {
  Serial.begin(9600);
  
  if (MODO_TESTE) {
    gerarDadosTeste();
  }

  // Recupera estado atual
  EEPROM.get(ADDR_INDICE, indiceAtual);
  EEPROM.get(ADDR_DADOS + (indiceAtual * sizeof(float)), consumoAcumulado);

  // Envia histórico completo para o Python ao iniciar
  enviarHistoricoSerial();
}

void loop() {
  // Simulação de leitura (Cálculo RMS simplificado para o exemplo)
  int leitura = analogRead(pinSensor);
  float correnteRMS = (leitura * (5.0 / 1023.0)) / 0.066; // Ex: Sensibilidade 66mV/A
  
  // Filtro de ruído
  if (correnteRMS < 0.15) correnteRMS = 0;

  unsigned long agora = millis();
  float horasDecorridas = (agora - ultimoCalculo) / 3600000.0;
  float potenciaW = correnteRMS * tensaoRede;
  
  consumoAcumulado += (potenciaW / 1000.0) * horasDecorridas;
  ultimoCalculo = agora;

  // Envio de dados em tempo real
  Serial.print("REAL_TIME|");
  Serial.print(correnteRMS, 3); Serial.print("|");
  Serial.print(potenciaW, 1); Serial.print("|");
  Serial.println(consumoAcumulado, 6);

  // Persistência horária
  if (agora - ultimoSalvamento >= INTERVALO_SALVAR) {
    indiceAtual = (indiceAtual + 1) % MAX_REGISTROS;
    EEPROM.put(ADDR_INDICE, indiceAtual);
    EEPROM.put(ADDR_DADOS + (indiceAtual * sizeof(float)), consumoAcumulado);
    ultimoSalvamento = agora;
  }
  
  delay(1000);
}

void enviarHistoricoSerial() {
  for (int i = 0; i < MAX_REGISTROS; i++) {
    float valor;
    EEPROM.get(ADDR_DADOS + (i * sizeof(float)), valor);
    Serial.print("HIST|");
    Serial.print(i); Serial.print("|");
    Serial.println(valor, 4);
    delay(10); // Evita saturação do buffer serial
  }
}

void gerarDadosTeste() {
  // Estimativa diária: Chuveiro (1.8kWh) + Máquina (2kWh) + Air Fryer (0.8kWh) + Base (4kWh) = ~8.6kWh/dia
  float baseConsumo = 0.0;
  for (int i = 0; i < MAX_REGISTROS; i++) {
    // Incremento aleatório simulando cargas variáveis por hora
    float incrementoHora = 0.2 + (random(0, 100) / 100.0) * 0.8; 
    baseConsumo += incrementoHora;
    EEPROM.put(ADDR_DADOS + (i * sizeof(float)), baseConsumo);
  }
  EEPROM.put(ADDR_INDICE, MAX_REGISTROS - 1);
}