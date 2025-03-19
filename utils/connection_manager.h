#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"    
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "cJSON.h"

// Estrutura para armazenar as configurações de conexão
typedef struct {
    char *ssid;
    char *senha;
    char *api_host;
    uint16_t api_port;
    char *api_url;
} wifi_config_t;

// Estrutura para armazenar os dados da conexão TCP
typedef struct {
    struct tcp_pcb *pcb;
    char *request;
    uint16_t request_len;
    bool complete;
    bool success;
    wifi_config_t *config; // Adicionado para ter acesso ao config
} tcp_connection_t;

// Declaração de funções auxiliares
// static void dns_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
static err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_error_callback(void *arg, err_t err);

// Função para inicializar o WiFi
bool wifi_init(wifi_config_t *config) {
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar o WiFi\n");
        return false;
    }
    
    cyw43_arch_enable_sta_mode();
    printf("Tentando conectar ao WiFi: %s\n", config->ssid);
    
    if (cyw43_arch_wifi_connect_timeout_ms(config->ssid, config->senha, 
                                          CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao WiFi\n");
        return false;
    }
    
    printf("WiFi conectado! IP: %s\n", 
           ip4addr_ntoa(netif_ip4_addr(netif_list)));
    
    return true;
}

// Função para verificar se o WiFi está conectado
bool wifi_is_connected() {
    return cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP;
}

// Função para tentar reconectar o WiFi se estiver desconectado
bool wifi_reconnect_if_needed(wifi_config_t *config) {
    if (!wifi_is_connected()) {
        printf("WiFi desconectado. Tentando reconectar...\n");
        
        cyw43_arch_enable_sta_mode();
        if (cyw43_arch_wifi_connect_timeout_ms(config->ssid, config->senha, 
                                              CYW43_AUTH_WPA2_AES_PSK, 1000)) {
            printf("Falha ao reconectar ao WiFi\n");
            return false;
        }
        
        printf("WiFi reconectado com sucesso!\n");
    }
    
    return true;
}

// Callbacks para a conexão TCP
static err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    tcp_connection_t *conn = (tcp_connection_t*)arg;
    
    if (err != ERR_OK) {
        printf("Falha na conexão TCP: %d\n", err);
        conn->success = false;
        conn->complete = true;
        return err;
    }
    
    printf("Conexão TCP estabelecida\n");
    
    // Enviar a requisição HTTP
    err = tcp_write(tpcb, conn->request, conn->request_len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Falha no envio da requisição: %d\n", err);
        conn->success = false;
        conn->complete = true;
        return err;
    }
    
    tcp_output(tpcb);
    return ERR_OK;
}

static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    tcp_connection_t *conn = (tcp_connection_t*)arg;
    
    if (p == NULL) {
        // Conexão fechada
        conn->complete = true;
        return ERR_OK;
    }
    
    if (p->tot_len > 0) {
        // Processar a resposta HTTP
        char *response = (char*)p->payload;
        
        // Verificar se a resposta foi bem-sucedida (HTTP 200 OK)
        if (strstr(response, "HTTP/1.1 200") != NULL || 
            strstr(response, "HTTP/1.0 200") != NULL) {
            printf("Requisição bem-sucedida\n");
            conn->success = true;
        } else {
            printf("Resposta do servidor: %s\n", response);
            conn->success = false;
        }
        
        tcp_recved(tpcb, p->tot_len);
    }
    
    pbuf_free(p);
    
    // Fechar a conexão após receber a resposta
    tcp_close(tpcb);
    conn->complete = true;
    
    printf("Fechei a conexao aqui!!\n");

    return ERR_OK;
}

static void tcp_error_callback(void *arg, err_t err) {
    tcp_connection_t *conn = (tcp_connection_t*)arg;
    printf("Erro na conexão TCP: %d\n", err);
    conn->success = false;
    conn->complete = true;
}

// Callback para resolução DNS
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    tcp_connection_t *conn = (tcp_connection_t*)callback_arg;
    
    if (ipaddr == NULL) {
        printf("Falha na resolução DNS para %s\n", name);
        conn->success = false;
        conn->complete = true;
        return;
    }
    
    // Acessar config através da estrutura de conexão
    wifi_config_t *config = conn->config;
    
    // Conectar ao servidor
    err_t err = tcp_connect(conn->pcb, ipaddr, config->api_port, tcp_connected_callback);
    if (err != ERR_OK) {
        printf("Falha ao iniciar conexão TCP: %d\n", err);
        tcp_close(conn->pcb);
        conn->success = false;
        conn->complete = true;
    }
}

// Função para enviar uma mensagem JSON para a API
bool send_json_to_api(wifi_config_t *config, const char *json_str) {
    // Verificar se o WiFi está conectado
    if (!wifi_is_connected()) {
        if (!wifi_reconnect_if_needed(config)) {
            return false;
        }
    }
    
    // Preparar a estrutura de conexão
    tcp_connection_t conn;
    conn.complete = false;
    conn.success = false;
    conn.config = config;  // Armazenar o ponteiro para config
    
    // Criar o PCB TCP
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Falha ao criar PCB TCP\n");
        return false;
    }
    
    // Configurar a conexão
    conn.pcb = pcb;
    
    // Preparar a requisição HTTP
    char request[1024];
    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             config->api_url, config->api_host, strlen(json_str), json_str);
    
    conn.request = request;
    conn.request_len = strlen(request);
    
    // Configurar callbacks
    tcp_arg(pcb, &conn);
    // tcp_sent(pcb, tcp_connected_callback);  // Corrigido aqui
    tcp_recv(pcb, tcp_recv_callback);
    tcp_err(pcb, tcp_error_callback);
    
    // Resolver o nome do host
    ip_addr_t remote_addr;
    
    // Tentar resolver o nome do host
    err_t err = dns_gethostbyname(config->api_host, &remote_addr, 
                                 dns_callback, &conn);
    
    if (err == ERR_INPROGRESS) {
        // Aguardar a resolução DNS
        while (!conn.complete) {
            cyw43_arch_poll();
            printf("Conexão ainda nao foi completa...\n");
            sleep_ms(10);
        }
    } else if (err != ERR_OK) {
        printf("Falha na resolução DNS: %d\n", err);
        tcp_close(pcb);
        return false;
    } else {
        // Conectar ao servidor
        err = tcp_connect(pcb, &remote_addr, config->api_port, tcp_connected_callback);
        if (err != ERR_OK) {
            printf("Falha ao iniciar conexão TCP: %d\n", err);
            tcp_close(pcb);
            return false;
        }
        
        // Aguardar a conclusão da conexão
        while (!conn.complete) {
            cyw43_arch_poll();
            printf("tentando conectar ainda...\n");
            sleep_ms(10);
        }
    }
    
    return conn.success;
}

// Função para enviar alerta usando cJSON
bool enviar_alerta_json(wifi_config_t *config, const char *device_id, 
                        int temperatura, int temp_max, int temp_min) {
    // Criar objeto JSON
    cJSON *alerta = cJSON_CreateObject();

    // TODO: Ajeitar dps do teste
    // cJSON_AddStringToObject(alerta, "dispositivo_id", device_id);
    // cJSON_AddNumberToObject(alerta, "temperatura", temperatura);
    // cJSON_AddNumberToObject(alerta, "temp_max", temp_max);
    // cJSON_AddNumberToObject(alerta, "temp_min", temp_min);
    
    cJSON_AddStringToObject(alerta, "status", "DETECTED");
    cJSON_AddStringToObject(alerta, "sensorIdentifier", "S01");


    // Converter para string
    char *json_str = cJSON_Print(alerta);
    printf("JSON enviado: %s", json_str);

    // Enviar para a API
    bool resultado = send_json_to_api(config, json_str);
    
    // Limpar recursos
    cJSON_Delete(alerta);
    free(json_str);
    free(alerta);
    
    return resultado;
}

#endif // CONNECTION_MANAGER_H