//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

/* A generic ringbuffer implementation */
#include "c_defs.h"
#include "conf.h"
#include "global.h"
#include "defs.h"

#include "rb.h"

/* flush (empty) a ringbuffer */
void rbFlush(ringBuffer_t *RB)
{
    if (RB)
    {
        RB->rp = RB->wp = RB->data;
        RB->ndata = 0;
    }

    return;
}


/* create a ringbuffer of a specified length */
ringBuffer_t *rbCreate(unsigned int len)
{
    ringBuffer_t *RB;

    if (!len)
        return NULL;

    if ( !(RB = (ringBuffer_t *)malloc(sizeof(ringBuffer_t))) )
        return NULL;

    if ( !(RB->data = (uint8_t *)malloc(len)) )
    {                           /* oops */
        free(RB);
        return NULL;
    }

    RB->len = len;                /* save the length */

    rbFlush(RB);                  /* init to empty */

    return RB;
}

/* destroy a ring buffer */
void rbDestroy(ringBuffer_t *RB)
{
    if (!RB)
        return;

    if (RB->data)
        free(RB->data);

    free(RB);

    return;
}

/* return the number of bytes in use in the rb */
unsigned int rbBytesUsed(ringBuffer_t *RB)
{
    if (RB)
        return RB->ndata;
    else
        return 0;
}

/* return the number of bytes available in the rb */
unsigned int rbBytesFree(ringBuffer_t *RB)
{
    if (RB)
        return (RB->len - RB->ndata);
    else
        return 0;
}

/* put data into a ring buffer */
unsigned int rbPut(ringBuffer_t *RB, uint8_t *buf, unsigned int len)
{
    unsigned int left, wlen = len, i;
    uint8_t *rptr = buf;

    if (!RB || !rptr)
        return 0;

    left = rbBytesFree(RB); /* max space available */

    if (wlen > left)
        wlen = left;

    for (i=0; i < wlen; i++, rptr++)
    {
        if ( RB->wp >= (RB->data + RB->len) )
        {
            RB->wp = RB->data;
        }

        *RB->wp = *rptr;
        RB->ndata++;
        RB->wp++;
    }

    /* return amount written */
    return wlen;
}

/* get and/or remove data from a ring buffer */
unsigned int rbGet(ringBuffer_t *RB, uint8_t *buf, unsigned int len, int update)
{
    uint8_t *wptr = buf, *rptr;
    unsigned int rlen = len, tlen, ndata;

    if (!len)
        return 0;

    rptr  = RB->rp;
    ndata = RB->ndata;

    if (rlen > ndata)
        rlen = ndata;

    tlen = rlen;

    while (rlen--)
    {
        if (rptr >= (RB->data + RB->len))
            rptr = RB->data;

        /* a NULL write ptr doesn't copy data */
        if (wptr)
        {
            *wptr = *rptr;
            wptr++;
        }

        rptr++;
        ndata--;
    }

    /* if we wanted to really remove the returned data from the rb, do it. */
    if (update)
    {
        RB->rp    = rptr;
        RB->ndata = ndata;
    }

    return(tlen);
}
