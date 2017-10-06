#ifndef __RingBuffer_H__
#define __RingBuffer_H__

#include <stdlib.h>
#include <stdio.h>
#include "StreamReader.h"
#include "BufferWriter.h"


/* ==========================================================================
 *
 *
 *
 * ========================================================================== */

class RingBuffer : public StreamReader, public BufferWriter {

private:

    // TODO: Add description
    BC capacity_;


protected:

    /* === To be implemented by sub-classes: =============================== */

    // TODO: Add description
    virtual void clear_() = 0;

    /* ===================================================================== */


public:

    // TODO: Add description
    RingBuffer();

    // TODO: Add description
    RingBuffer(const BC& capacity);


    // TODO: Add description
    void clear();

    // TODO: Add description
    BC getCapacity() const;

    // TODO: Add description
    BC getRemainingCapacity() const;


    // TODO: Add description
    virtual ~RingBuffer();

};

#endif
