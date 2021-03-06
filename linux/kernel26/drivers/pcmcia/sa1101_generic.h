#include "soc_common.h"
#include "sa11xx_base.h"

extern int sa1101_pcmcia_hw_init(struct soc_pcmcia_socket *);
extern void sa1101_pcmcia_hw_shutdown(struct soc_pcmcia_socket *);
extern void sa1101_pcmcia_socket_state(struct soc_pcmcia_socket *, struct pcmcia_state *);
extern int sa1101_pcmcia_configure_socket(struct soc_pcmcia_socket *, const socket_state_t *);
extern void sa1101_pcmcia_socket_init(struct soc_pcmcia_socket *);
extern void sa1101_pcmcia_socket_suspend(struct soc_pcmcia_socket *);

extern int pcmcia_jornada820_init(struct device *);
