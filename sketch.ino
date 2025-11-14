// ========================================================================
// FOCUSTOGETHER - SISTEMA DE MONITORAMENTO DE AMBIENTE DE TRABALHO
// Global Solution 2025 - Edge Computing

// Sistema de monitoramento de luminosidade e postura ergonômica
// para profissionais em trabalho remoto, utilizando Edge Computing para
// processamento local e preservação de privacidade.
// Medimos a luminosidade do ambiente e a distância que o usúario está da tela

// João Victor (RM: 566640)
// Gustavo Macedo (RM: 567594)
// Yan Lucas (RM: 567046)

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Inicializa LCD 16x2 com endereço I2C 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);


// definindo os pinos

const int PIN_LDR = A0;              // Sensor de luminosidade 
const int PIN_TRIG = 3;              // Trigger do sensor ultrassônico
const int PIN_ECHO = 2;              // Echo do sensor ultrassônico
const int PIN_LED_VERDE = 9;         // LED indicador OURO (ideal)
const int PIN_LED_AMARELO = 10;      // LED indicador PRATA (atenção)
const int PIN_LED_VERMELHO = 11;     // LED indicador BRONZE (alerta)
const int PIN_BOTAO = 4;             // Botão para pausar/retomar


// constantes de classificação - luminosidade

// LDR no Arduino - quanto MAIS luz, MAIOR o valor (0-1023)
// Contrário de um fotoresistor comum isolado

const int LUZ_BRONZE_MAX = 300;      // Abaixo de 300 = escuro (BRONZE)
const int LUZ_PRATA_MIN = 300;       // Entre 300-700 = adequado (PRATA)
const int LUZ_PRATA_MAX = 700;
const int LUZ_OURO_MIN = 700;        // Acima de 700 = ideal (OURO)


// constantes de classificação - postura ergonômica

// baseado em normas de ergonomia a distância ideal da tela é de 40-60cm

const int POSTURA_MUITO_PROXIMO = 25;     // < 25cm = muito perto (BRONZE)
const int POSTURA_IDEAL_MIN = 40;         // 40-60cm = ideal (OURO)
const int POSTURA_IDEAL_MAX = 60;
const int POSTURA_LONGE = 80;             // > 80cm = muito longe (BRONZE)


// ESTRUTURA DE DADOS DO SISTEMA
// usamos struct para guardar as 6 informações juntas
struct SistemaStatus {
  int leitura_luz;                    // valor bruto do LDR (0-1023)
  String classificacao_luz;           // BRONZE/PRATA/OURO
  int distancia_cm;                   // distância em centímetros
  String classificacao_postura;       // BRONZE/PRATA/OURO
  String classificacao_final;         // classificação geral do ambiente
  String recomendacao;                // Mensagem para o usuário
};


// variáveis globais
SistemaStatus status_sistema;
unsigned long tempo_ultimo_botao = 0;
const int DEBOUNCE = 300;             // tempo de debounce do botão (ms)
bool monitorando = true;              // estado do sistema (ativo/pausado)
bool estado_pausado_exibido = false;  // controla exibição da tela de pausa


// caracteres customizados

byte emojiFeliz[8] = {
  B00000,
  B01010,
  B01010,
  B00000,
  B10001,
  B10001,
  B01110,
  B00000
};

byte emojiSerio[8] = {
  B00000,
  B01010,
  B01010,
  B00000,
  B00000,
  B11111,
  B00000,
  B00000
};

byte emojiTriste[8] = {
  B00000,
  B01010,
  B01010,
  B00000,
  B01110,
  B10001,
  B10001,
  B00000
};


// inicialização do sistema

void setup() {
  // inicializa comunicação serial
  Serial.begin(9600);
  
  // inicializa LCD e backlight
  lcd.init();
  lcd.backlight();
  
  // carrega caracteres customizados
  lcd.createChar(0, emojiFeliz);
  lcd.createChar(1, emojiSerio);
  lcd.createChar(2, emojiTriste);
  
  // configuração dos pinos
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AMARELO, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BOTAO, INPUT); 
  pinMode(PIN_LDR, INPUT);
  
  // apaga todos os LEDs
  apagarTodosLEDs();
  
  // exibe tela inicial
  telaInicial();
  
  // mensagens iniciais no Serial Monitor
  Serial.println("\n========================================");
  Serial.println("  FOCUSTOGETHER - SISTEMA INICIADO");
  Serial.println("========================================");
  Serial.println("Monitoramento de Luminosidade + Postura");
  Serial.println("Pressione o botao para pausar/retomar");
  Serial.println("========================================\n");
}

// loop principal

void loop() {
  // verifica se o botão foi pressionado
  verificarBotao();
  
  // se sistema estiver pausado, exibe apenas a tela de pausa
  if (!monitorando) {
    if (!estado_pausado_exibido) {
      mostrarTelaPausa();
      estado_pausado_exibido = true;
    }
    delay(100);
    return;
  }

  // sistema ativo - executa monitoramento completo
  estado_pausado_exibido = false;
  
  lerLuminosidade();
  lerDistancia();
  classificarLuz();
  classificarPostura();
  classificacaoFinal();
  atualizarLEDs();
  atualizarLCD();
  imprimirStatus();

  delay(500);  // atualiza a cada 500ms
}


// função da tela inicial

void telaInicial() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("FocusTogether");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(1500);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema Pronto!");
  lcd.setCursor(0, 1);
  lcd.print("Monitorando...");
  delay(1000);
}

// função para verificar o botão

void verificarBotao() {
  int leitura_botao = digitalRead(PIN_BOTAO);
  
  // botão pressionado = HIGH
  if (leitura_botao == HIGH) {
    unsigned long tempo_atual = millis();
    
    // verifica o  debounce (evita múltiplas leituras)
    if (tempo_atual - tempo_ultimo_botao > DEBOUNCE) {
      tempo_ultimo_botao = tempo_atual;
      
      // alterna estado de monitoramento
      monitorando = !monitorando;
      
      if (!monitorando) {
        Serial.println("\n========================================");
        Serial.println(">>> SISTEMA PAUSADO <<<");
        Serial.println("Pressione novamente para retomar");
        Serial.println("========================================\n");
      } else {
        Serial.println("\n========================================");
        Serial.println(">>> SISTEMA RETOMADO <<<");
        Serial.println("Monitoramento ativo");
        Serial.println("========================================\n");
        
        // animação de retomada do sistema
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Retomando...");
        lcd.setCursor(0, 1);
        lcd.print("Aguarde...");
        delay(800);
      }
      
      // aguarda botão ser solto
      while (digitalRead(PIN_BOTAO) == HIGH) {
        delay(10);
      }
    }
  }
}

// função da tela de pausa

void mostrarTelaPausa() {
  lcd.clear();
  apagarTodosLEDs();
  
  lcd.setCursor(0, 0);
  lcd.print(" MONITORAMENTO");
  lcd.setCursor(0, 1);
  lcd.print("    PAUSADO");
  
  Serial.println(">>> Tela pausada - LEDs apagados <<<");
}

// função para ler luminosidade

void lerLuminosidade() {
  // lê valor analógico do LDR (0-1023)
  // quanto MAIS luz, MAIOR o valor
  status_sistema.leitura_luz = analogRead(PIN_LDR);
}

// função para ler a distância (sensor HC-SR04)
void lerDistancia() {
  // envia pulso de trigger
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  
  // lê o tempo de resposta do echo
  long duracao_microsegundos = pulseIn(PIN_ECHO, HIGH, 30000);
  
  // converte tempo em distância (velocidade do som = 343 m/s)
  status_sistema.distancia_cm = duracao_microsegundos * 0.0343 / 2;
  
  // limita leitura máxima (alcance do sensor HC-SR04: 2-400cm)
  if (status_sistema.distancia_cm > 400 || status_sistema.distancia_cm == 0) {
    status_sistema.distancia_cm = 999;  // Indica "fora de alcance"
  }
}

// função para classificar a luminosidade

void classificarLuz() {
  if (status_sistema.leitura_luz < LUZ_BRONZE_MAX) {
    // abaixo de 300 = Ambiente ESCURO
    status_sistema.classificacao_luz = "BRONZE";
  } 
  else if (status_sistema.leitura_luz >= LUZ_OURO_MIN) {
    // acima de 700 = Iluminação IDEAL
    status_sistema.classificacao_luz = "OURO";
  } 
  else {
    // entre 300-700 = Iluminação ADEQUADA
    status_sistema.classificacao_luz = "PRATA";
  }
}

// função para classificar a postura

void classificarPostura() {
  if (status_sistema.distancia_cm < POSTURA_MUITO_PROXIMO) {
    // menos de 25cm = MUITO PERTO (prejudica visão)
    status_sistema.classificacao_postura = "BRONZE";
  }
  else if (status_sistema.distancia_cm >= POSTURA_IDEAL_MIN && 
           status_sistema.distancia_cm <= POSTURA_IDEAL_MAX) {
    // entre 40-60cm = DISTÂNCIA IDEAL (normas ergonômicas)
    status_sistema.classificacao_postura = "OURO";
  }
  else if ((status_sistema.distancia_cm >= POSTURA_MUITO_PROXIMO && 
            status_sistema.distancia_cm < POSTURA_IDEAL_MIN) ||
           (status_sistema.distancia_cm > POSTURA_IDEAL_MAX && 
            status_sistema.distancia_cm <= POSTURA_LONGE)) {
    // entre 25-40cm OU 60-80cm = ATENÇÃO (ajuste recomendado)
    status_sistema.classificacao_postura = "PRATA";
  }
  else {
    // acima de 80cm = MUITO LONGE (postura inadequada)
    status_sistema.classificacao_postura = "BRONZE";
  }
}

// função para classificar o ambiente no geral

void classificacaoFinal() {
  // A lógica prioriza o pior caso (BRONZE > PRATA > OURO)
  // Se qualquer sensor está em BRONZE, ambiente é BRONZE
  
  if (status_sistema.classificacao_luz == "BRONZE" || 
      status_sistema.classificacao_postura == "BRONZE") {
    status_sistema.classificacao_final = "BRONZE";
    status_sistema.recomendacao = "Ajuste urgente!";
  } 
  else if (status_sistema.classificacao_luz == "PRATA" || 
           status_sistema.classificacao_postura == "PRATA") {
    status_sistema.classificacao_final = "PRATA";
    status_sistema.recomendacao = "Atencao!";
  } 
  else {
    status_sistema.classificacao_final = "OURO";
    status_sistema.recomendacao = "Ambiente ideal!";
  }
}

// função para atualizar leds

void atualizarLEDs() {
  apagarTodosLEDs();
  
  if (status_sistema.classificacao_final == "BRONZE") {
    digitalWrite(PIN_LED_VERMELHO, HIGH);  // Vermelho = ALERTA
  } 
  else if (status_sistema.classificacao_final == "PRATA") {
    digitalWrite(PIN_LED_AMARELO, HIGH);   // Amarelo = ATENÇÃO
  } 
  else if (status_sistema.classificacao_final == "OURO") {
    digitalWrite(PIN_LED_VERDE, HIGH);     // Verde = IDEAL
  }
}

// função para apagar todos os leds

void apagarTodosLEDs() {
  digitalWrite(PIN_LED_VERMELHO, LOW);
  digitalWrite(PIN_LED_AMARELO, LOW);
  digitalWrite(PIN_LED_VERDE, LOW);
}

// função para atualizar o display

void atualizarLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);

  // linha 1: Status + Emoji
  lcd.print(status_sistema.classificacao_final);
  lcd.print(" ");
  
  if (status_sistema.classificacao_final == "BRONZE") {
    lcd.write((byte)2);  // Emoji triste
    lcd.print(" ALERTA");
  }
  else if (status_sistema.classificacao_final == "PRATA") {
    lcd.write((byte)1);  // Emoji sério
    lcd.print(" OK");
  }
  else if (status_sistema.classificacao_final == "OURO") {
    lcd.write((byte)0);  // Emoji feliz
    lcd.print(" IDEAL");
  }

  // linha 2: Recomendação ou detalhes
  lcd.setCursor(0, 1);
  
  // Exibe informações detalhadas alternadas
  static unsigned long tempo_ultima_alternancia = 0;
  static bool mostra_luz = true;
  
  if (millis() - tempo_ultima_alternancia > 3000) {  // Alterna a cada 3s
    mostra_luz = !mostra_luz;
    tempo_ultima_alternancia = millis();
  }
  
  if (mostra_luz) {
    lcd.print("Luz:");
    lcd.print(status_sistema.leitura_luz);
    lcd.print(" (");
    lcd.print(status_sistema.classificacao_luz.substring(0, 3));  // Primeiras 3 letras
    lcd.print(")");
  } else {
    lcd.print("Dist:");
    if (status_sistema.distancia_cm < 999) {
      lcd.print(status_sistema.distancia_cm);
      lcd.print("cm (");
      lcd.print(status_sistema.classificacao_postura.substring(0, 3));
      lcd.print(")");
    } else {
      lcd.print("---");
    }
  }
}

// função para mostrar status no serial monitor

void imprimirStatus() {
  Serial.print("Luz: ");
  Serial.print(status_sistema.leitura_luz);
  Serial.print(" (");
  Serial.print(status_sistema.classificacao_luz);
  Serial.print(") | Dist: ");
  
  if (status_sistema.distancia_cm < 999) {
    Serial.print(status_sistema.distancia_cm);
    Serial.print("cm");
  } else {
    Serial.print("---");
  }
  
  Serial.print(" (");
  Serial.print(status_sistema.classificacao_postura);
  Serial.print(") | Final: ");
  Serial.print(status_sistema.classificacao_final);
  Serial.print(" - ");
  Serial.println(status_sistema.recomendacao);
}
