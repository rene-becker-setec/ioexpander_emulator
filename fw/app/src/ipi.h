#ifndef __IPI_H__
#define __IPI_H__


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start IPI service
 *
 * @return int
 */
int ipi_init(void);

/**
 * @brief enqueue a CAM Message for transmission
 *
 * @return int
 */
int ipe_send_can_msg(void);

#ifdef __cplusplus
}
#endif

#endif
