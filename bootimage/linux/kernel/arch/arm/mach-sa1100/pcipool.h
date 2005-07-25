/* Jornada820 version based on pcipool.h 1.1 from cvs.handhelds.org
 * $Id: pcipool.h,v 1.2 2005/07/25 09:09:07 fare Exp $
 */

struct pci_pool *pci_pool_create (const char *name, struct pci_dev *dev,
		size_t size, size_t align, size_t allocation, int flags);
void pci_pool_destroy (struct pci_pool *pool);

void *pci_pool_alloc (struct pci_pool *pool, int flags, dma_addr_t *handle);
void pci_pool_free (struct pci_pool *pool, void *vaddr, dma_addr_t addr);

