#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

bool wifi_init_sta(void);
void get_device_service_name(char *service_name, size_t max);
void get_proof_of_possession(char *pop, size_t max);

#endif // WIFI_PROVISIONING_H
