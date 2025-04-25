# Bibliotecas
import streamlit as st
import pandas as pd
import numpy as np
import base64
import plotly.express as px
import plotly.graph_objects as go
from datetime import datetime, time, timedelta



# Configuração de página para usar todo o espaço disponível
st.set_page_config(layout="wide")

# Carregando o dataset simulado (substituir pelo banco de dados real proveniente do servidor flask)
@st.cache_data
def criar_dataframe():
    df = pd.read_csv('dados_simulados_fev-abril.csv')
    return df
df = criar_dataframe()

# Converte a coluna timestamp de string para datetime (caso ainda esteja em string)
df["timestamp"] = pd.to_datetime(df["timestamp"]) 
df.set_index("timestamp", inplace=True)  # garantir que é datetime

# Pega o momento atual para bases de tempo relativas ao usuário
#now = datetime.now().date()
now = datetime.now().date() + timedelta(days=1) # +1 dia para garantir que o dia atual seja incluído



# --- CSS geral para o dashboard ---
# Personalização de formatação, barra lateral e dos botões
st.markdown(
    """
    <style>
        
        /* Cor de fundo do dashboard */
        .stApp {
            background-color: #F0F8FF;  /* Cor de fundo do dashboard: azul escuro */
        }

        /* Cor dos subtítulos */
        h3 {
            color: black !important;
        }

        
        
        /* Estilo do título (label) de cada .metric() */
        [data-testid="stMetricLabel"] p {
            display: flex !important;
            position: relative !important;
            left: -8px !important;
            justify-content: center !important;
            width: 100% !important;
            font-size: 20px !important;
            font-weight: bold;
            color: black !important;  /* Força a cor preta */
        }
            
        /* Configuração dos textos (value) dentro de cada .metric() */
        [data-testid="stMetricValue"] > div {
            text-align: center !important;
            font-size: 30px !important;
            color: black !important;  /* Força a cor preta */
        }

        /* Configuração dos containers de cada .metric() */
        [data-testid="stMetric"] {
            display: flex !important;
            flex-direction: column !important;
            align-items: center !important;
            justify-content: center !important;
            width: 100% !important; 
            padding: 10px;
            border-radius: 15px;
            background-color: #D0F0F7; /* Fundo azul claro */
            box-shadow: 0px 4px 12px rgba(0, 0, 0, 0.3); /* Sombra ao redor dos containers */   
        }   

        

        /* Removendo padding e espaçamentos padrão */
        .main .block-container {
            padding-top: 2rem;
            padding-bottom: 2rem;
            padding-left: 3rem;
            padding-right: 3rem;
        }

         

        /* Estilo para os gráficos Plotly */
        [data-testid="stPlotlyChart"] {
            border-radius: 15px; /* Bordas arredondadas */
            overflow: hidden;    /* Esconde o conteúdo que ultrapassa os limites do gráfico */
            box-shadow: 0px 4px 12px rgba(0, 0, 0, 0.3); /* Sombra ao redor do gráfico */
            margin-top: -15px !important;
            margin-bottom: 15px !important;
        }

    </style>
    """,
    unsafe_allow_html=True,
)
# --- Fim do CSS geral ---



# Função para converter uma imagem para base64
def get_image_base64(path):
    with open(path, "rb") as img_file:
        return base64.b64encode(img_file.read()).decode()

# Carregando imagem (logo) e convertendo para base64
image_path = "logo_Safevax.png"
image_base64 = get_image_base64(image_path)

# HTML/CSS para alinhar imagem e título
st.markdown(
    f"""
    <div style="display: flex; justify-content: center; align-items: center; gap: 50px;">
        <img src="data:image/png;base64,{image_base64}" style="height: 110px; width: auto;">
        <h1 style="
            font-size: 35px;
            font-weight: bold;
            color: black;
            margin: 0;
            text-align: left;
            font-family: 'Arial', sans-serif;
        ">SAFEVAX: Monitoramento de Vacinas</h1>
    </div>
    """,
    unsafe_allow_html=True
)

# Linha divisória
st.markdown("<hr style='border: 2.5px solid black;"
                       "margin-top: 2rem;" 
                       "margin-bottom: 1.0rem;'>",
            unsafe_allow_html=True)


# --- Filtro de data ---
col_txt_filtro, col_filtro = st.columns([2, 3], gap='small')

# CSS para alinhamento e largura do selectbox
st.markdown(
    """
    <style>
        /* Alinha o conteúdo da segunda coluna à esquerda */
        div[data-testid="column"]:nth-of-type(2) {
            align-items: flex-start !important;
            justify-content: flex-start !important;
        }
        
        /* Ajusta a largura do selectbox */
        [data-testid="stSelectbox"] > div {
            width: fit-content !important;
            min-width: 250px !important;
            font-size: 18px !important;
            color: black !important;  /* Força a cor preta */
            font-weight: bold !important;
            margin-left: -100px !important;  /* Margem à esquerda */
            margin-top: 10px !important;  /* Margem acima */
            margin-bottom: 5px !important;  /* Margem abaixo */
    }
    </style>
    """, 
    unsafe_allow_html=True)

with col_txt_filtro:
    st.markdown("""
    <h3 style='
        font-size:26px;
        color:black;
        margin-bottom: -15px;  /* Reduz espaço abaixo do título */
    '>Selecione o período a ser observado:</h3>
    """, unsafe_allow_html=True)

with col_filtro:
    periodo = st.selectbox(
        "",
        ("Hoje", "Última semana", "Último mês", "Customizado"),
        label_visibility="collapsed"
    )


# 2) Determina intervalo
if periodo == "Customizado":
    # Limita seleção ao range disponível no dataset
    dataset_min = df.index.min().date()
    dataset_max = df.index.max().date()
    default_start = max(dataset_min, now - timedelta(days=30))
    default_end = min(dataset_max, now)
    date_selection = st.date_input(
        "INTERVALO CUSTOMIZADO",
        value=[default_start, default_end],
        min_value=dataset_min,
        max_value=dataset_max
    )
    # Ajusta início e fim mesmo se for seleção única
    if isinstance(date_selection, datetime):
        start_date = end_date = date_selection.date()
    elif isinstance(date_selection, (list, tuple)) and len(date_selection) == 1:
        start_date = end_date = date_selection[0]
    else:
        start_date, end_date = date_selection
    start_dt = datetime.combine(start_date, time.min)
    end_dt = datetime.combine(end_date, time.max)
else:
    # para períodos rápidos, baseamos em 'now', não no dataset
    if periodo == "Hoje":
        start_dt = now - timedelta(days=1)
    elif periodo == "Última semana":
        start_dt = now - timedelta(weeks=1)
    else:  # Último mês
        start_dt = now - timedelta(days=30)
    end_dt = now

# 3) Subconjunto do período escolhido
df_period = df.loc[start_dt:end_dt]
# --- Fim do filtro de data ---



# --- Agrupamento dos dados em grupos menores ---
# 4) Define agregação dinâmica
delta = end_dt - start_dt
if delta <= timedelta(days=2):
    rule = "15min"
elif delta <= timedelta(days=7):
    rule = "1H"
else:
    rule = "24H"

# 5) Resample com agregações
df_resampled = df_period.resample(rule).agg({
    "DHT_interno_temp": "max",
    "DHT_interno_umidade": "mean",
    "DHT_externo_temp": "max",
    "DHT_externo_umidade": "mean",
    "Motivo":       lambda x: x.mode()[0] if not x.empty else np.nan,
    "Estado_alerta":lambda x: x.mode()[0] if not x.empty else np.nan,
    "estado_porta": lambda x: x.mode()[0] if not x.empty else np.nan
}).round(2).reset_index()
# --- Fim do agrupamento ---



# --- Definindo variáveis para gráficos e visualizações de texto ---
# 6) Hovertext para as linhas temporais
def make_hover(r):
    return (
        f"<b>Temperatura interna máxima:</b> {r['DHT_interno_temp']}°C<br>"
        f"<b>Horário e data:</b> {r['timestamp'].strftime('%H:%M (%d/%m/%Y)')}<br>"
        f"<b>Estado da porta:</b> {r['estado_porta']}"
    )

hover_in = df_resampled.apply(make_hover, axis=1)
hover_ex = df_resampled.apply(
    lambda r: (
        f"<b>Temperatura externa média:</b> {r['DHT_externo_temp']}°C<br>"
        f"<b>Horário e data:</b> {r['timestamp'].strftime('%H:%M (%d/%m/%Y)')}<br>"
        f"<b>Estado da porta:</b> {r['estado_porta']}"
    ), axis=1
)


if rule == "24H":
    hover_in = df_resampled.apply(
        lambda r: (
            f"<b>Dia analisado:</b> {r['timestamp'].strftime('%d/%m/%Y')}<br>"
            f"<b>Temperatura interna máxima:</b> {r['DHT_interno_temp']}°C"
        ), axis=1
    )


# 7) Processa alertas no nível original
alerts = df_period[(df_period['Estado_alerta']=='ALERT') & (df_period['Motivo']=='temp_high')]
if rule == "24H":
    alerts = alerts.copy()
    alerts['date'] = alerts.index.date
    counts = alerts.groupby('date').size().rename('Quantidade').reset_index()
    alerts['timestamp'] = alerts.index
    alerts_sorted = alerts.sort_values('DHT_interno_temp', ascending=False)
    top = alerts_sorted.groupby('date').first().reset_index()
    alert_points = top.merge(counts, on='date')
    alert_points['timestamp'] = pd.to_datetime(alert_points['date'])
else:
    alert_points = alerts.copy()
    alert_points['timestamp'] = alert_points.index

# 8) Hovertext para alertas
if rule == "24H":
    hover_alert = alert_points.apply(
        lambda r: (
            f"<b>Alerta:</b> Temperatura elevada<br>"
            f"<b>Dia analisado:</b> {r['timestamp'].strftime('%d/%m/%Y')}<br>"
            f"<b>Quantidade de alertas:</b> {r['Quantidade']}<br>"
            f"<b>Temperatura interna máxima:</b> {r['DHT_interno_temp']}°C"
        ), axis=1
    )
else:
    hover_alert = alert_points.apply(
        lambda r: (
            f"<b>Alerta:</b> Temperatura elevada<br>"
            f"<b>Horário e data:</b> {r['timestamp'].strftime('%H:%M (%d/%m/%Y)')}<br>"
            f"<b>Temperatura interna detectada:</b> {r['DHT_interno_temp']}°C<br>"
            f"<b>Estado da porta:</b> {r['estado_porta']}"
        ), axis=1
    )
# --- Fim das variáveis para gráficos e visualizações de texto ---



# --- Quatro métricas relevantes ---
with st.container():
    # Texto de introdução das métricas
    st.markdown("<h3 style='font-size:26px;"
                        "color:black;'"    
            ">❄️📊 Resumo de Desempenho do Refrigerador</h3>",
            unsafe_allow_html=True)

    alerts_metrics = df_period[(df_period['Motivo']=='unauthorized_door') | (df_period['Motivo']=='temp_high')]

    # Primeira linha com 4 colunas
    colA, colB, colC, colD = st.columns(4)

    # A. Temperatura interna máxima no período
    temp_max = df_period["DHT_interno_temp"].max()
    colA.metric("🔹 Temperatura interna máxima", f"{temp_max:.1f} °C")

    # B. Quantidade total de alertas por temperatura alta ou porta aberta sem autorização
    qtd_alertas = alerts_metrics.shape[0]
    colB.metric("🔹 Quantidade de alertas", qtd_alertas)

    # C. Último alerta (se houver)
    mapeamento_motivos = {  # Dicionário de mapeamento para os motivos
        'temp_high': 'Temperatura Elevada',
        'unauthorized_door': 'Acesso não autorizado'
        }
    
    # Verifica se há alertas para exibir o último alerta
    if not alerts_metrics.empty:
        ultimo_alerta = alerts_metrics.iloc[-1]
        hora_data = ultimo_alerta.name.strftime("%H:%M (%d/%m)")
        motivo = ultimo_alerta["Motivo"]
        colC.metric("🔹 Último alerta", f"{hora_data}", help=f"Motivo: {mapeamento_motivos.get(motivo, motivo)}")
    else:
        colC.metric("🔹 Último alerta", "Nenhum")

    # D. Total de aberturas de porta
    qtd_aberturas = df_period[df_period["estado_porta"] == "Aberta"].shape[0]
    colD.metric("🔹 Total de Aberturas de Porta", qtd_aberturas)
    # --- Fim das métricas relevantes ---



# --- Gráficos principais ---
# Divisão de espaço para dois gráficos
col1, col2 = st.columns([1100, 500], gap='small')

with col1:
    st.markdown("<h3 style='font-size:26px;"
                           "color:black;" 
                           "margin-top:5px;'"    
    ">🌡️📅 Monitoramento das Temperaturas e Histórico de Acesso</h3>",
    unsafe_allow_html=True)
    
    # CSS para fixar a largura do primeiro gráfico
    st.markdown(
        """
        <style>
            div[data-testid="column"]:nth-of-type(1) {
                min-width: 1100px !important;
                max-width: 1100px !important;
            }
        </style>
        """, unsafe_allow_html=True)
    

    # ----------------------------------- GRÁFICO 1 ----------------------------------- #
    # 9) Montagem gráfico temporal
    fig = go.Figure()

    # Adiciona linha temporal de temperatura externa 
    fig.add_trace(go.Scatter(
        x=df_resampled['timestamp'], 
        y=df_resampled['DHT_externo_temp'],
        mode='lines', 
        name='Temperatura externa',
        hovertext=hover_ex, 
        hoverinfo='text', 
        line=dict(color='#00695C') # Verde petróleo escurecido
    ))

    # Adiciona linha temporal de temperatura interna
    fig.add_trace(go.Scatter(
        x=df_resampled['timestamp'], 
        y=df_resampled['DHT_interno_temp'],
        mode='lines', 
        name='Temperatura interna',
        hovertext=hover_in, 
        hoverinfo='text', 
        line=dict(color='#1565C0', width=2.3, dash='dash' if rule == "24H" else 'solid') # Azul claro escurecido
    ))

    # Configura ticks do eixo X
    if delta <= timedelta(days=2):
        xaxis_cfg = dict(tickformat='%H:%M', dtick=3*3600*1000, tickfont=dict(size=14, color='black', family='Arial'))
    else:
        xaxis_cfg = dict(tickformat='%d/%m', dtick=24*3600*1000, tickfont=dict(size=15, color='black'))
    
    # Adiciona pontos de alerta (se houver)
    fig.add_trace(go.Scatter(
        x=alert_points['timestamp'], y=alert_points['DHT_interno_temp'],
        mode='markers', name='Temperatura elevada',
        marker=dict(size=8, color='red', symbol='circle', opacity=0.75),
        hovertext=hover_alert, hoverinfo='text'
    ))


    fig.update_layout(
        title={
            'text': 'Temperatura Interna e Externa do Refrigerador',
            'font': {'size': 23, 'color': 'black'},
            'x': 0.03,  # Pequena margem da esquerda
            'xanchor': 'left'
        },

        paper_bgcolor='#D0F0F7',  # Cor de fundo ao redor do gráfico
        plot_bgcolor='rgba(0,0,0,0)',  # Fundo transparente
        font=dict(color='black'),  # Texto em branco para contraste
        margin=dict(l=80, r=180, t=80, b=50),  # Ajuste equilibrado das margens

        # Personalização da fonte do eixo Y
        yaxis=dict(
            title='Temperatura (°C)',
            tickfont=dict(
                size=15,
                color='black',
                family='Arial',
            ),
            showgrid=True,
            gridcolor='rgba(0,0,0,0.4)'
        ),

        # Personalização dos rótulos dos eixos X e Y
        xaxis=xaxis_cfg,
        xaxis_title='Período',
        xaxis_title_font=dict(size=18, color='black'), 
        yaxis_title_font=dict(size=18, color='black'),

        # Personalização da legenda
        legend=dict(
            font=dict(
                size=13,
                color='black',
            ),
            x=1.01,
            y=1
        ),

        width=1100,  # Largura do gráfico

        hoverlabel=dict(bgcolor='white', font_color='black', font_size=15),  # Texto ao passar o mouse
    )

    # 10) Exibe no Streamlit
    st.plotly_chart(fig, use_container_width=True)
    # ----------------------------------- FINAL GRÁFICO 1 ----------------------------------- #



# ----------------------------------- GRÁFICO 2 ----------------------------------- #
with col2:
    # CSS para fixar a largura do segundo gráfico
    st.markdown(
        """
        <style>
            div[data-testid="column"]:nth-of-type(1) {
                min-width: 500px !important;
                max-width: 500px !important;
            }
        </style>
        """, unsafe_allow_html=True)
    
    # 11) Montagem gráfico dataset
    # Título
    st.markdown(
    """
    <h3 style='font-size:22.5px;
               color:black;
               font-weight:bold;
               text-align:center;
               padding-top:65px; 
               margin-bottom:-25px;'
    >Registro de Aberturas de Porta</h3>
    """,
    unsafe_allow_html=True
)

    # Filtra apenas registros com estado_porta == 'Aberta'
    df_aberturas = df_period[df_period["estado_porta"] == "Aberta"]

    # Reseta o índice para transformar timestamp em coluna
    df_aberturas_view = (
        df_aberturas
        .reset_index()[["timestamp", "usuario"]]  # Agora timestamp é coluna
        .sort_values("timestamp", ascending=False)
        .reset_index(drop=True)  # Limpa o índice antigo
    )

    # Formatação de data (HH:MM (DD/MM/YYYY))
    df_aberturas_view["timestamp"] = df_aberturas_view["timestamp"].dt.strftime("%H:%M (%d/%m/%Y)")

    # Substitui valores de usuário
    df_aberturas_view["usuario"] = df_aberturas_view["usuario"].replace({
        "desconhecido": "Desconhecido"
    })

    # Renomeia as colunas
    df_aberturas_view = df_aberturas_view.rename(columns={
        "timestamp": "HORÁRIO E DATA",
        "usuario":   "USUÁRIO"
    })

    def highlight_row(row):
        # Se "Usuário" == 'Desconhecido', fundo vermelho, senão verde 
        color = 'background-color: #E57373' if row['USUÁRIO'] == 'Desconhecido' else 'background-color: #81C784' # Vermelho:  #EF5350
        return [color] * len(row)                                                                                # Verde:  #66BB6A

    # Aplica o estilo de cores linha a linha
    styled = df_aberturas_view.style.apply(highlight_row, axis=1)

    # Exibe o dataset como tabela no dashboard
    st.dataframe(styled, use_container_width=True)
    # ----------------------------------- FINAL GRÁFICO 2 ----------------------------------- #

