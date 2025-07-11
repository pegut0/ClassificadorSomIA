# Passo 1: Instale as dependências no seu ambiente local
# pip install Flask tensorflow numpy librosa soundfile

import os
import numpy as np
import librosa
import tensorflow as tf
from flask import Flask, request, jsonify
import soundfile as sf
import traceback 
import io 

# --- Configurações Globais ---
SR = 22050
DURATION = 5
N_MELS = 128
HOP_LENGTH = 512
CLARITY_THRESHOLD = 0.3 
SILENCE_THRESHOLD = 0.01
STATIONARITY_THRESHOLD = 4.0

# --- Inicialização do Flask ---
app = Flask(__name__)
app.config['MAX_CONTENT_LENGTH'] = 2 * 1024 * 1024 

# --- Carregar o Modelo e Mapeamento de Classes ---
try:
    MODEL_PATH = "classificador_som_otimizado.keras"
    if not os.path.exists(MODEL_PATH):
        raise FileNotFoundError(f"Arquivo do modelo não encontrado: {MODEL_PATH}. Verifique o caminho.")
    
    model = tf.keras.models.load_model(MODEL_PATH)
    print("✅ Modelo carregado com sucesso!")

    # Os nomes internos continuam em inglês, pois o modelo foi treinado com eles
    class_names = ['clock_alarm', 'crying_baby', 'dog', 'door_wood_knock', 'glass_breaking', 'siren']
    idx_to_class = {idx: cls_name for idx, cls_name in enumerate(class_names)}
    print(f"✅ Classes reconhecidas: {class_names}")

    # --- MUDANÇA: Dicionário de Tradução ---
    TRANSLATIONS_PT = {
        'clock_alarm': 'Alarme de Relógio',
        'crying_baby': 'Bebe Chorando',
        'dog': 'Cachorro',
        'door_wood_knock': 'Batida na Porta',
        'glass_breaking': 'Vidro Quebrando',
        'siren': 'Sirene',
        'Nao reconhecido': 'Nao reconhecido'  
    }

except Exception as e:
    print(f"❌ Erro ao carregar o modelo: {e}")
    model = None
    idx_to_class = {}

# --- Função de Extração de Features ---
def extract_features_for_prediction(y, sr):
    try:
        target_length = DURATION * SR
        if len(y) < target_length:
            y = np.pad(y, (0, target_length - len(y)), mode='constant')
        else:
            y = y[:target_length]

        mel = librosa.feature.melspectrogram(y=y, sr=sr, n_mels=N_MELS, hop_length=HOP_LENGTH)
        mel_db = librosa.power_to_db(mel, ref=np.max)
        return mel_db
    except Exception as e:
        print(f"Erro ao extrair features: {e}")
        return None

# --- Rota da API para Predição (com mais logs de erro) ---
@app.route('/predict', methods=['POST'])
def predict():
    if model is None: return jsonify({"error": "Modelo não está carregado."}), 500
    if not request.data: return jsonify({"error": "Nenhum dado de áudio recebido."}), 400
    
    audio_data = request.data
    
    try:
        audio_file_in_memory = io.BytesIO(audio_data)
        y, sr_orig = sf.read(audio_file_in_memory)
        
        if y.ndim > 1: y = y.mean(axis=1)
        if sr_orig != SR: y = librosa.resample(y, orig_sr=sr_orig, target_sr=SR)
        
        # --- Filtro 1: Detecção de Silêncio ---
        rms = librosa.feature.rms(y=y)[0]
        if np.mean(rms) < SILENCE_THRESHOLD:
            print(f"Áudio muito baixo (RMS: {np.mean(rms):.4f}). Rejeitando como silêncio.")
            response = {"prediction": "Nao reconhecido", "confidence": "0.00"}
            return jsonify(response)

        # --- Filtro 2: Detecção de Som Estacionário (Ventilador) ---
        mel_db_pred = extract_features_for_prediction(y, SR)
        if mel_db_pred is None: return jsonify({"error": "Falha ao processar as features."}), 500
        
        temporal_std_dev = np.std(mel_db_pred, axis=1)
        mean_std_dev = np.mean(temporal_std_dev)

        if mean_std_dev < STATIONARITY_THRESHOLD:
            print(f"Som muito constante (Variação: {mean_std_dev:.2f}). Rejeitando como ruído de fundo.")
            response = {"prediction": "Nao reconhecido", "confidence": "0.00"}
            return jsonify(response)

        # --- Filtro 3: Predição da IA e Análise de Clareza ---
        input_data = mel_db_pred[np.newaxis, ..., np.newaxis]
        pred_probs = model.predict(input_data)[0]
        
        top_two_indices = pred_probs.argsort()[-2:][::-1]
        top_prob_idx, second_top_prob_idx = top_two_indices[0], top_two_indices[1]

        top_confidence = pred_probs[top_prob_idx]
        second_top_confidence = pred_probs[second_top_prob_idx]

        prediction = idx_to_class[top_prob_idx]
        confidence = top_confidence

        if (top_confidence - second_top_confidence) < CLARITY_THRESHOLD:
            print(f"Predição incerta: {prediction} ({top_confidence:.2f}) vs {idx_to_class[second_top_prob_idx]} ({second_top_confidence:.2f}). Rejeitando.")
            prediction = "Nao reconhecido"
        
        # --- MUDANÇA: Traduz a predição final ---
        prediction_pt = TRANSLATIONS_PT.get(prediction, prediction)

        response = {
            "prediction": prediction_pt, 
            "confidence": f"{confidence:.2f}"
        }
        
        print(f"Request recebido, predição final: {prediction_pt} com confiança {confidence:.2f}")
        return jsonify(response)
        
    except Exception as e:
        print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
        print("!!! OCORREU UM ERRO AO PROCESSAR O ARQUIVO DE ÁUDIO !!!")
        print(f"!!! Tipo do Erro: {type(e).__name__}")
        print(f"!!! Mensagem: {e}")
        traceback.print_exc() 
        print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
        return jsonify({"error": "Falha ao processar o arquivo de audio no servidor."}), 500

# --- Rota Principal (para teste no navegador) ---
@app.route("/")
def index():
    return "<h1>Servidor de Classificação de Som está no ar! (v5 - Traduzido)</h1>"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
