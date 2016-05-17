/*
 * Copyright 2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * pmemobj_list.h -- macro definitions for persistent
 * singly linked list and tail queue
 */

#ifndef PMEMOBJ_LISTS_H
#define PMEMOBJ_LISTS_H

#include <libpmemobj.h>

/*
 * This file defines two types of persistent data structures:
 * singly-linked lists and tail queue.
 *
 * All macros defined in this file must be used within libpmemobj
 * transactional API. Following snippet presents example of usage:
 *
 *	TX_BEGIN(pop) {
 *		POBJ_TAILQ_INIT(head);
 *	} TX_ONABORT {
 *		abort();
 *	} TX_END
 *
 *                              SLIST    TAILQ
 * _HEAD                        +        +
 * _ENTRY                       +        +
 * _INIT                        +        +
 * _EMPTY                       +        +
 * _FIRST                       +        +
 * _NEXT                        +        +
 * _PREV                        -        +
 * _LAST                        -        +
 * _FOREACH                     +        +
 * _FOREACH_REVERSE             -        +
 * _INSERT_HEAD                 +        +
 * _INSERT_BEFORE               -        +
 * _INSERT_AFTER                +        +
 * _INSERT_TAIL                 -        +
 * _MOVE_ELEMENT_HEAD           -        +
 * _MOVE_ELEMENT_TAIL           -        +
 * _REMOVE_HEAD                 +        -
 * _REMOVE                      +        +
 * _REMOVE_FREE                 +        +
 * _SWAP_HEAD_TAIL              -        +
 */

/*
 * Singly-linked List definitions.
 */
#define POBJ_SLIST_HEAD(name, type)\
struct name {\
	TOID(type) pe_first;\
}

#define POBJ_SLIST_ENTRY(type)\
struct {\
	TOID(type) pe_next;\
}

/*
 * Singly-linked List access methods.
 */
#define POBJ_SLIST_EMPTY(head)	(TOID_IS_NULL(D_RO(head)->pe_first))
#define POBJ_SLIST_FIRST(head)	(D_RO(head)->pe_first)
#define POBJ_SLIST_NEXT(elm, field)	(D_RO(elm)->field.pe_next)

/*
 * Singly-linked List functions.
 */
#define POBJ_SLIST_INIT(head) do {\
	TX_ADD_FIELD(head, pe_first);\
	TOID_ASSIGN(D_RW(head)->pe_first, OID_NULL);\
} while (0)

#define POBJ_SLIST_INSERT_HEAD(head, elm, field) do {\
	D_RW(elm)->field.pe_next = D_RO(head)->pe_first;\
	TX_SET(head, pe_first, elm);\
} while (0)

#define POBJ_SLIST_INSERT_AFTER(slistelm, elm, field) do {\
	TX_ADD(slistelm);\
	D_RW(elm)->field.pe_next = D_RO(slistelm)->field.pe_next;\
	D_RW(slistelm)->field.pe_next = elm;\
} while (0)

#define POBJ_SLIST_REMOVE_HEAD(head, field) do {\
		TX_ADD(head);\
		D_RW(head)->pe_first =\
			D_RO(D_RO(head)->pe_first)->field.pe_next;\
} while (0)

#define POBJ_SLIST_REMOVE(head, elm, field) do {\
	if (TOID_EQUALS(D_RO(head)->pe_first, elm)) {\
		POBJ_SLIST_REMOVE_HEAD(head, field);\
	} else {\
		typeof(elm) curelm = D_RO(head)->pe_first;\
		while (!TOID_EQUALS(D_RO(curelm)->field.pe_next, elm))\
			curelm = D_RO(curelm)->field.pe_next;\
		TX_ADD(curelm);\
		D_RW(curelm)->field.pe_next\
				= D_RO(elm)->field.pe_next;\
	}\
} while (0)

#define POBJ_SLIST_REMOVE_FREE(head, elm, field) do {\
	POBJ_SLIST_REMOVE(head, elm, field);\
	TX_FREE(elm);\
} while (0)

#define POBJ_SLIST_FOREACH(var, head, field)\
	for ((var) = POBJ_SLIST_FIRST(head);\
		!TOID_IS_NULL(var);\
		var = POBJ_SLIST_NEXT(var, field))

/*
 * Tail-queue definitions.
 */
#define POBJ_TAILQ_ENTRY(type)\
struct {\
	TOID(type) pe_next;\
	TOID(type) pe_prev;\
}

#define POBJ_TAILQ_HEAD(name, type)\
struct name {\
	TOID(type) pe_first;\
	TOID(type) pe_last;\
}

/*
 * Tail-queue access methods.
 */
#define POBJ_TAILQ_FIRST(head)	(D_RW(head)->pe_first)
#define POBJ_TAILQ_LAST(head)	(D_RW(head)->pe_last)

#define POBJ_TAILQ_EMPTY(head)	(TOID_IS_NULL(D_RO(head)->pe_first))
#define POBJ_TAILQ_NEXT(elm, field)	(D_RO(elm)->field.pe_next)
#define POBJ_TAILQ_PREV(elm, field)	(D_RO(elm)->field.pe_prev)

/*
 * Tail-queue List internal methods.
 */
#define _POBJ_SWAP_PTR(elm, field) do {\
	TX_ADD(elm);\
	typeof(elm) temp = D_RO(elm)->field.pe_prev;\
	D_RW(elm)->field.pe_prev = D_RW(elm)->field.pe_next;\
	D_RW(elm)->field.pe_next = temp;\
} while (0)

/*
 * Tail-queue functions.
 */
#define POBJ_TAILQ_SWAP_HEAD_TAIL(head, field) do {\
	typeof(D_RO(head)->pe_first) temp = D_RO(head)->pe_first;\
	TX_SET(head, pe_first, D_RO(head)->pe_last);\
	TX_SET(head, pe_last, temp);\
} while (0)

#define POBJ_TAILQ_FOREACH(var, head, field)\
	for ((var) = POBJ_TAILQ_FIRST(head);\
		!TOID_IS_NULL(var);\
		var = POBJ_TAILQ_NEXT(var, field))

#define POBJ_TAILQ_FOREACH_REVERSE(var, head, field)\
	for ((var) = POBJ_TAILQ_LAST(head);\
		!TOID_IS_NULL(var);\
		var = POBJ_TAILQ_PREV(var, field))

#define POBJ_TAILQ_INIT(head) do {\
	TX_ADD_FIELD(head, pe_first);\
	TOID_ASSIGN(D_RW(head)->pe_first, OID_NULL);\
	TX_ADD_FIELD(head, pe_last);\
	TOID_ASSIGN(D_RW(head)->pe_last, OID_NULL);\
} while (0)

#define POBJ_TAILQ_INSERT_HEAD(head, elm, field) do {\
	if (TOID_IS_NULL(D_RO(head)->pe_first)) {\
		D_RW(elm)->field.pe_prev = D_RO(head)->pe_first;\
		D_RW(elm)->field.pe_next = D_RO(head)->pe_first;\
		TX_SET(head, pe_first, elm);\
		TX_SET(head, pe_last, elm);\
	} else {\
		D_RW(elm)->field.pe_next = D_RO(head)->pe_first;\
		D_RW(elm)->field.pe_prev =\
			D_RO(D_RO(head)->pe_first)->field.pe_prev;\
		TX_ADD(D_RO(head)->pe_first);\
		D_RW(D_RW(head)->pe_first)->field.pe_prev = elm;\
		TX_SET(head, pe_first, elm);\
	}\
} while (0)

#define POBJ_TAILQ_INSERT_TAIL(head, elm, field) do {\
	if (TOID_IS_NULL(D_RO(head)->pe_last)) {\
		D_RW(elm)->field.pe_prev = D_RO(head)->pe_last;\
		D_RW(elm)->field.pe_next = D_RO(head)->pe_last;\
		TX_SET(head, pe_first, elm);\
		TX_SET(head, pe_last, elm);\
	} else {\
		D_RW(elm)->field.pe_prev = D_RO(head)->pe_last;\
		D_RW(elm)->field.pe_next =\
			D_RO(D_RO(head)->pe_last)->field.pe_next;\
		TX_ADD(D_RO(head)->pe_last);\
		D_RW(D_RW(head)->pe_last)->field.pe_next = elm;\
		TX_SET(head, pe_last, elm);\
	}\
} while (0)

#define POBJ_TAILQ_INSERT_AFTER(listelm, elm, field) do {\
	D_RW(elm)->field.pe_prev = listelm;\
	D_RW(elm)->field.pe_next = D_RO(listelm)->field.pe_next;\
	TX_ADD(listelm);\
	if (TOID_IS_NULL(D_RO(listelm)->field.pe_next)) {\
		TX_SET(head, pe_last, elm);\
	} else {\
		TX_ADD(D_RO(listelm)->field.pe_next);\
		D_RW(D_RW(listelm)->field.pe_next)->field.pe_prev = elm;\
	}\
	D_RW(listelm)->field.pe_next = elm;\
} while (0)

#define POBJ_TAILQ_INSERT_BEFORE(listelm, elm, field) do {\
	D_RW(elm)->field.pe_next = listelm;\
	D_RW(elm)->field.pe_prev = D_RO(listelm)->field.pe_prev;\
	TX_ADD(listelm);\
	if (TOID_IS_NULL(D_RO(listelm)->field.pe_prev)) {\
		TX_SET(head, pe_first, elm);\
	} else {\
		TX_ADD(D_RO(listelm)->field.pe_prev);\
		D_RW(D_RW(listelm)->field.pe_prev)->field.pe_next = elm; \
	}\
	D_RW(listelm)->field.pe_prev = elm;\
} while (0)

#define POBJ_TAILQ_REMOVE(head, elm, field) do {\
	if (TOID_IS_NULL(D_RO(elm)->field.pe_prev) &&\
		TOID_IS_NULL(D_RO(elm)->field.pe_next)) {\
		TX_SET(head, pe_first, D_RO(elm)->field.pe_prev);\
		TX_SET(head, pe_last, D_RO(elm)->field.pe_next);\
	} else {\
		if (TOID_IS_NULL(D_RO(elm)->field.pe_prev)) {\
			TX_SET(head, pe_first, D_RO(elm)->field.pe_next);\
			TX_ADD(D_RO(elm)->field.pe_next);\
			D_RW(D_RW(elm)->field.pe_next)->field.pe_prev =\
				D_RO(elm)->field.pe_prev;\
		} else {\
			TX_ADD(D_RO(elm)->field.pe_prev);\
			D_RW(D_RW(elm)->field.pe_prev)->field.pe_next = \
				D_RO(elm)->field.pe_next;\
		}\
		if (TOID_IS_NULL(D_RO(elm)->field.pe_next)) {\
			TX_SET(head, pe_last, D_RO(elm)->field.pe_prev);\
			TX_ADD(D_RO(elm)->field.pe_prev);\
			D_RW(D_RW(elm)->field.pe_prev)->field.pe_next = \
				D_RO(elm)->field.pe_next;\
		} else {\
			TX_ADD(D_RO(elm)->field.pe_next);\
			D_RW(D_RW(elm)->field.pe_next)->field.pe_prev =\
				D_RO(elm)->field.pe_prev;\
		}\
	}\
} while (0)

#define POBJ_TAILQ_REMOVE_FREE(head, elm, field) do {\
	POBJ_TAILQ_REMOVE(head, elm, field);\
	TX_FREE(elm);\
} while (0)

/*
 * 2 cases: only two elements, the rest possibilities
 * including that elm is the last one
 */
#define POBJ_TAILQ_MOVE_ELEMENT_HEAD(head, elm, field) do {\
	if (TOID_EQUALS(D_RO(head)->pe_last, elm) &&\
		TOID_EQUALS(D_RO(D_RO(head)->pe_first)->field.pe_next, elm)) {\
		_POBJ_SWAP_PTR(elm, field);\
		_POBJ_SWAP_PTR(D_RW(head)->pe_first, field);\
		POBJ_TAILQ_SWAP_HEAD_TAIL(head, field);\
	} else {\
		TX_ADD(elm);\
		TX_ADD(D_RO(elm)->field.pe_prev);\
		TX_ADD(D_RO(head)->pe_first);\
		D_RW(D_RW(elm)->field.pe_prev)->field.pe_next =\
			D_RO(elm)->field.pe_next;\
		if (TOID_EQUALS(D_RO(head)->pe_last, elm)) {\
			TX_SET(head, pe_last, D_RO(elm)->field.pe_prev);\
		} else {\
			TX_ADD(D_RO(elm)->field.pe_next);\
			D_RW(D_RW(elm)->field.pe_next)->field.pe_prev = \
				D_RO(elm)->field.pe_prev;\
		}\
		D_RW(elm)->field.pe_prev =\
			D_RO(D_RO(head)->pe_first)->field.pe_prev;\
		D_RW(elm)->field.pe_next = D_RO(head)->pe_first;\
		D_RW(D_RW(head)->pe_first)->field.pe_prev = elm;\
		TX_SET(head, pe_first, elm);\
	}\
} while (0)

#define POBJ_TAILQ_MOVE_ELEMENT_TAIL(head, elm, field) do {\
	if (TOID_EQUALS(D_RO(head)->pe_first, elm) &&\
		TOID_EQUALS(D_RO(D_RO(head)->pe_last)->field.pe_prev, elm)) {\
		_POBJ_SWAP_PTR(elm, field);\
		_POBJ_SWAP_PTR(D_RW(head)->pe_last, field);\
		POBJ_TAILQ_SWAP_HEAD_TAIL(head, field);\
	} else {\
		TX_ADD(elm);	\
		TX_ADD(D_RO(elm)->field.pe_next);\
		TX_ADD(D_RO(head)->pe_last);\
		D_RW(D_RW(elm)->field.pe_next)->field.pe_prev =\
			D_RO(elm)->field.pe_prev;\
		if (TOID_EQUALS(D_RO(head)->pe_first, elm)) {\
			TX_SET(head, pe_first, D_RO(elm)->field.pe_next);\
		} else {	\
			TX_ADD(D_RO(elm)->field.pe_prev);\
			D_RW(D_RW(elm)->field.pe_prev)->field.pe_next =\
				D_RO(elm)->field.pe_next;\
		}\
		D_RW(elm)->field.pe_prev = D_RO(head)->pe_last;\
		D_RW(elm)->field.pe_next = \
			D_RO(D_RO(head)->pe_last)->field.pe_next;\
		D_RW(D_RW(head)->pe_last)->field.pe_next = elm;\
		TX_SET(head, pe_last, elm);\
	}	\
} while (0)

#endif /* PMEMOBJ_LISTS_H */