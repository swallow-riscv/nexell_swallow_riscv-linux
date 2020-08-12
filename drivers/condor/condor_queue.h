#ifndef LIBCONDOR_CONDOR_QUEUE_H
#define LIBCONDOR_CONDOR_QUEUE_H

#include "condor.h"

/* 송신큐에 버퍼링 가능한 패킷 최대 개수 */
#define	CONDOR_TXQ_PKT_MAXNUM			100
/* 송신큐 Almost Full 이벤트 임계점 */
#define	CONDOR_TXQ_ALMOST_FULL_PKTNUM	(CONDOR_TXQ_PKT_MAXNUM - 10)
/* 송신큐 Almost Empty 이벤트 임계점 */
#define	CONDOR_TXQ_ALMOST_EMPTY_PKTNUM	10

/* 수신큐에 버퍼링 가능한 패킷 최대 개수 */
#define	CONDOR_RXQ_PKT_MAXNUM			100

#ifdef QUEUE_MACRO_DEBUG
/* Store the last 2 places the queue element or head was altered */
struct qm_trace {
	unsigned long	 lastline;
	unsigned long	 prevline;
	const char	*lastfile;
	const char	*prevfile;
};

#undef TRACEBUF
#define	TRACEBUF	struct qm_trace trace;
#undef TRACEBUF_INITIALIZER
#define	TRACEBUF_INITIALIZER	{ __FILE__, __LINE__, NULL, 0 } ,
#undef TRASHIT
#define	TRASHIT(x)	do {(x) = (void *)-1;} while (0)
#undef QMD_SAVELINK
#define	QMD_SAVELINK(name, link)	void **name = (void *)&(link)

#undef QMD_TRACE_HEAD
#define	QMD_TRACE_HEAD(head) do {					\
	(head)->trace.prevline = (head)->trace.lastline;		\
	(head)->trace.prevfile = (head)->trace.lastfile;		\
	(head)->trace.lastline = __LINE__;				\
	(head)->trace.lastfile = __FILE__;				\
} while (0)

#undef QMD_TRACE_ELEM
#define	QMD_TRACE_ELEM(elem) do {					\
	(elem)->trace.prevline = (elem)->trace.lastline;		\
	(elem)->trace.prevfile = (elem)->trace.lastfile;		\
	(elem)->trace.lastline = __LINE__;				\
	(elem)->trace.lastfile = __FILE__;				\
} while (0)

#else
#undef QMD_TRACE_ELEM
#define	QMD_TRACE_ELEM(elem)
#undef QMD_TRACE_HEAD
#define	QMD_TRACE_HEAD(head)
#undef QMD_SAVELINK
#define	QMD_SAVELINK(name, link)
#undef TRACEBUF
#define	TRACEBUF
#undef TRACEBUF_INITIALIZER
#define	TRACEBUF_INITIALIZER
#undef TRASHIT
#define	TRASHIT(x)
#endif	/* QUEUE_MACRO_DEBUG */


#if 0
/*
 * Singly-linked List declarations.
 */
#undef SLIST_HEAD
#define	SLIST_HEAD(name, type)						\
struct name {								\
	struct type *slh_first;	/* first element */			\
}

#undef SLIST_HEAD_INITIALIZER
#define	SLIST_HEAD_INITIALIZER(head)					\
	{ NULL }

#undef SLIST_ENTRY
#define	SLIST_ENTRY(type)						\
struct {								\
	struct type *sle_next;	/* next element */			\
}

/*
 * Singly-linked List functions.
 */
#undef SLIST_EMPTY
#define	SLIST_EMPTY(head)	((head)->slh_first == NULL)

#undef SLIST_FIRST
#define	SLIST_FIRST(head)	((head)->slh_first)

#undef SLIST_FOREACH
#define	SLIST_FOREACH(var, head, field)					\
	for ((var) = SLIST_FIRST((head));				\
	    (var);							\
	    (var) = SLIST_NEXT((var), field))

#undef SLIST_FOREACH_FROM
#define	SLIST_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : SLIST_FIRST((head)));		\
	    (var);							\
	    (var) = SLIST_NEXT((var), field))

#undef SLIST_FOREACH_SAFE
#define	SLIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = SLIST_FIRST((head));				\
	    (var) && ((tvar) = SLIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#undef SLIST_FOREACH_FROM_SAFE
#define	SLIST_FOREACH_FROM_SAFE(var, head, field, tvar)			\
	for ((var) = ((var) ? (var) : SLIST_FIRST((head)));		\
	    (var) && ((tvar) = SLIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#undef SLIST_FOREACH_PREVPTR
#define	SLIST_FOREACH_PREVPTR(var, varp, head, field)			\
	for ((varp) = &SLIST_FIRST((head));				\
	    ((var) = *(varp)) != NULL;					\
	    (varp) = &SLIST_NEXT((var), field))

#undef SLIST_INIT
#define	SLIST_INIT(head) do {						\
	SLIST_FIRST((head)) = NULL;					\
} while (0)

#undef SLIST_INSERT_AFTER
#define	SLIST_INSERT_AFTER(slistelm, elm, field) do {			\
	SLIST_NEXT((elm), field) = SLIST_NEXT((slistelm), field);	\
	SLIST_NEXT((slistelm), field) = (elm);				\
} while (0)

#undef SLIST_INSERT_HEAD
#define	SLIST_INSERT_HEAD(head, elm, field) do {			\
	SLIST_NEXT((elm), field) = SLIST_FIRST((head));			\
	SLIST_FIRST((head)) = (elm);					\
} while (0)

#undef SLIST_NEXT
#define	SLIST_NEXT(elm, field)	((elm)->field.sle_next)

#undef SLIST_REMOVE
#define	SLIST_REMOVE(head, elm, type, field) do {			\
	QMD_SAVELINK(oldnext, (elm)->field.sle_next);			\
	if (SLIST_FIRST((head)) == (elm)) {				\
		SLIST_REMOVE_HEAD((head), field);			\
	}								\
	else {								\
		struct type *curelm = SLIST_FIRST((head));		\
		while (SLIST_NEXT(curelm, field) != (elm))		\
			curelm = SLIST_NEXT(curelm, field);		\
		SLIST_REMOVE_AFTER(curelm, field);			\
	}								\
	TRASHIT(*oldnext);						\
} while (0)

#undef SLIST_REMOVE_AFTER
#define SLIST_REMOVE_AFTER(elm, field) do {				\
	SLIST_NEXT(elm, field) =					\
	    SLIST_NEXT(SLIST_NEXT(elm, field), field);			\
} while (0)

#undef SLIST_REMOVE_HEAD
#define	SLIST_REMOVE_HEAD(head, field) do {				\
	SLIST_FIRST((head)) = SLIST_NEXT(SLIST_FIRST((head)), field);	\
} while (0)

#undef SLIST_SWAP
#define SLIST_SWAP(head1, head2, type) do {				\
	struct type *swap_first = SLIST_FIRST(head1);			\
	SLIST_FIRST(head1) = SLIST_FIRST(head2);			\
	SLIST_FIRST(head2) = swap_first;				\
} while (0)

/*
 * Singly-linked Tail queue declarations.
 */
#undef STAILQ_HEAD
#define	STAILQ_HEAD(name, type)						\
struct name {								\
	struct type *stqh_first;/* first element */			\
	struct type **stqh_last;/* addr of last next element */		\
}

#undef STAILQ_HEAD_INITIALIZER
#define	STAILQ_HEAD_INITIALIZER(head)					\
	{ NULL, &(head).stqh_first }

#undef STAILQ_ENTRY
#define	STAILQ_ENTRY(type)						\
struct {								\
	struct type *stqe_next;	/* next element */			\
}

/*
 * Singly-linked Tail queue functions.
 */
#undef STAILQ_CONCAT
#define	STAILQ_CONCAT(head1, head2) do {				\
	if (!STAILQ_EMPTY((head2))) {					\
		*(head1)->stqh_last = (head2)->stqh_first;		\
		(head1)->stqh_last = (head2)->stqh_last;		\
		STAILQ_INIT((head2));					\
	}								\
} while (0)

#undef STAILQ_EMPTY
#define	STAILQ_EMPTY(head)	((head)->stqh_first == NULL)

#undef STAILQ_FIRST
#define	STAILQ_FIRST(head)	((head)->stqh_first)

#undef STAILQ_FOREACH
#define	STAILQ_FOREACH(var, head, field)				\
	for ((var) = STAILQ_FIRST((head));				\
	   (var);							\
	   (var) = STAILQ_NEXT((var), field))

#undef STAILQ_FOREACH_FROM
#define	STAILQ_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : STAILQ_FIRST((head)));		\
	   (var);							\
	   (var) = STAILQ_NEXT((var), field))

#undef STAILQ_FOREACH_SAFE
#define	STAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = STAILQ_FIRST((head));				\
	    (var) && ((tvar) = STAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#undef STAILQ_FOREACH_FROM_SAFE
#define	STAILQ_FOREACH_FROM_SAFE(var, head, field, tvar)		\
	for ((var) = ((var) ? (var) : STAILQ_FIRST((head)));		\
	    (var) && ((tvar) = STAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#undef STAILQ_INIT
#define	STAILQ_INIT(head) do {						\
	STAILQ_FIRST((head)) = NULL;					\
	(head)->stqh_last = &STAILQ_FIRST((head));			\
} while (0)

#undef STAILQ_INSERT_AFTER
#define	STAILQ_INSERT_AFTER(head, tqelm, elm, field) do {		\
	if ((STAILQ_NEXT((elm), field) = STAILQ_NEXT((tqelm), field)) == NULL)\
		(head)->stqh_last = &STAILQ_NEXT((elm), field);		\
	STAILQ_NEXT((tqelm), field) = (elm);				\
} while (0)

#undef STAILQ_INSERT_HEAD
#define	STAILQ_INSERT_HEAD(head, elm, field) do {			\
	if ((STAILQ_NEXT((elm), field) = STAILQ_FIRST((head))) == NULL)	\
		(head)->stqh_last = &STAILQ_NEXT((elm), field);		\
	STAILQ_FIRST((head)) = (elm);					\
} while (0)

#undef STAILQ_INSERT_TAIL
#define	STAILQ_INSERT_TAIL(head, elm, field) do {			\
	STAILQ_NEXT((elm), field) = NULL;				\
	*(head)->stqh_last = (elm);					\
	(head)->stqh_last = &STAILQ_NEXT((elm), field);			\
} while (0)

#undef STAILQ_LAST
#define	STAILQ_LAST(head, type, field)					\
	(STAILQ_EMPTY((head)) ? NULL :					\
	    __containerof((head)->stqh_last, struct type, field.stqe_next))

#undef STAILQ_NEXT
#define	STAILQ_NEXT(elm, field)	((elm)->field.stqe_next)

#undef STAILQ_REMOVE
#define	STAILQ_REMOVE(head, elm, type, field) do {			\
	QMD_SAVELINK(oldnext, (elm)->field.stqe_next);			\
	if (STAILQ_FIRST((head)) == (elm)) {				\
		STAILQ_REMOVE_HEAD((head), field);			\
	}								\
	else {								\
		struct type *curelm = STAILQ_FIRST((head));		\
		while (STAILQ_NEXT(curelm, field) != (elm))		\
			curelm = STAILQ_NEXT(curelm, field);		\
		STAILQ_REMOVE_AFTER(head, curelm, field);		\
	}								\
	TRASHIT(*oldnext);						\
} while (0)

#undef STAILQ_REMOVE_AFTER
#define STAILQ_REMOVE_AFTER(head, elm, field) do {			\
	if ((STAILQ_NEXT(elm, field) =					\
	     STAILQ_NEXT(STAILQ_NEXT(elm, field), field)) == NULL)	\
		(head)->stqh_last = &STAILQ_NEXT((elm), field);		\
} while (0)

#undef STAILQ_REMOVE_HEAD
#define	STAILQ_REMOVE_HEAD(head, field) do {				\
	if ((STAILQ_FIRST((head)) =					\
	     STAILQ_NEXT(STAILQ_FIRST((head)), field)) == NULL)		\
		(head)->stqh_last = &STAILQ_FIRST((head));		\
} while (0)

#undef STAILQ_SWAP
#define STAILQ_SWAP(head1, head2, type) do {				\
	struct type *swap_first = STAILQ_FIRST(head1);			\
	struct type **swap_last = (head1)->stqh_last;			\
	STAILQ_FIRST(head1) = STAILQ_FIRST(head2);			\
	(head1)->stqh_last = (head2)->stqh_last;			\
	STAILQ_FIRST(head2) = swap_first;				\
	(head2)->stqh_last = swap_last;					\
	if (STAILQ_EMPTY(head1))					\
		(head1)->stqh_last = &STAILQ_FIRST(head1);		\
	if (STAILQ_EMPTY(head2))					\
		(head2)->stqh_last = &STAILQ_FIRST(head2);		\
} while (0)


/*
 * List declarations.
 */
/* gyun@keti : 이 아래 구문은 include/linux/list.h의 LIST_HEAD를 undefine한다. 이로 인한 문제는 없을까? */
#undef LIST_HEAD
#define	LIST_HEAD(name, type)						\
struct name {								\
	struct type *lh_first;	/* first element */			\
}

#undef LIST_HEAD_INITIALIZER
#define	LIST_HEAD_INITIALIZER(head)					\
	{ NULL }

#undef LIST_ENTRY
#define	LIST_ENTRY(type)						\
struct {								\
	struct type *le_next;	/* next element */			\
	struct type **le_prev;	/* address of previous next element */	\
}

/*
 * List functions.
 */
#undef LIST_EMPTY
#define	LIST_EMPTY(head)	((head)->lh_first == NULL)

#undef LIST_FIRST
#define	LIST_FIRST(head)	((head)->lh_first)

#undef LIST_FOREACH
#define	LIST_FOREACH(var, head, field)					\
	for ((var) = LIST_FIRST((head));				\
	    (var);							\
	    (var) = LIST_NEXT((var), field))

#undef LIST_FOREACH_FROM
#define	LIST_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : LIST_FIRST((head)));		\
	    (var);							\
	    (var) = LIST_NEXT((var), field))

#undef LIST_FOREACH_SAFE
#define	LIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = LIST_FIRST((head));				\
	    (var) && ((tvar) = LIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#undef LIST_FOREACH_FROM_SAFE
#define	LIST_FOREACH_FROM_SAFE(var, head, field, tvar)			\
	for ((var) = ((var) ? (var) : LIST_FIRST((head)));		\
	    (var) && ((tvar) = LIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#undef LIST_INIT
#define	LIST_INIT(head) do {						\
	LIST_FIRST((head)) = NULL;					\
} while (0)

#undef LIST_INSERT_AFTER
#define	LIST_INSERT_AFTER(listelm, elm, field) do {			\
	if ((LIST_NEXT((elm), field) = LIST_NEXT((listelm), field)) != NULL)\
		LIST_NEXT((listelm), field)->field.le_prev =		\
		    &LIST_NEXT((elm), field);				\
	LIST_NEXT((listelm), field) = (elm);				\
	(elm)->field.le_prev = &LIST_NEXT((listelm), field);		\
} while (0)

#undef LIST_INSERT_BEFORE
#define	LIST_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.le_prev = (listelm)->field.le_prev;		\
	LIST_NEXT((elm), field) = (listelm);				\
	*(listelm)->field.le_prev = (elm);				\
	(listelm)->field.le_prev = &LIST_NEXT((elm), field);		\
} while (0)

#undef LIST_INSERT_HEAD
#define	LIST_INSERT_HEAD(head, elm, field) do {				\
	if ((LIST_NEXT((elm), field) = LIST_FIRST((head))) != NULL)	\
		LIST_FIRST((head))->field.le_prev = &LIST_NEXT((elm), field);\
	LIST_FIRST((head)) = (elm);					\
	(elm)->field.le_prev = &LIST_FIRST((head));			\
} while (0)

#undef LIST_NEXT
#define	LIST_NEXT(elm, field)	((elm)->field.le_next)

#undef LIST_PREV
#define	LIST_PREV(elm, head, type, field)				\
	((elm)->field.le_prev == &LIST_FIRST((head)) ? NULL :		\
	    __containerof((elm)->field.le_prev, struct type, field.le_next))

#undef LIST_REMOVE
#define	LIST_REMOVE(elm, field) do {					\
	QMD_SAVELINK(oldnext, (elm)->field.le_next);			\
	QMD_SAVELINK(oldprev, (elm)->field.le_prev);			\
	if (LIST_NEXT((elm), field) != NULL)				\
		LIST_NEXT((elm), field)->field.le_prev = 		\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = LIST_NEXT((elm), field);		\
	TRASHIT(*oldnext);						\
	TRASHIT(*oldprev);						\
} while (0)

#undef LIST_SWAP
#define LIST_SWAP(head1, head2, type, field) do {			\
	struct type *swap_tmp = LIST_FIRST((head1));			\
	LIST_FIRST((head1)) = LIST_FIRST((head2));			\
	LIST_FIRST((head2)) = swap_tmp;					\
	if ((swap_tmp = LIST_FIRST((head1))) != NULL)			\
		swap_tmp->field.le_prev = &LIST_FIRST((head1));		\
	if ((swap_tmp = LIST_FIRST((head2))) != NULL)			\
		swap_tmp->field.le_prev = &LIST_FIRST((head2));		\
} while (0)

#endif


/*
 * Tail queue declarations.
 */
#undef TAILQ_HEAD
#define	TAILQ_HEAD(name, type)						\
struct name {								\
	struct type *tqh_first;	/* first element */			\
	struct type **tqh_last;	/* addr of last next element */		\
	TRACEBUF							\
}

#undef TAILQ_HEAD_INITIALIZER
#define	TAILQ_HEAD_INITIALIZER(head)					\
	{ NULL, &(head).tqh_first, TRACEBUF_INITIALIZER }

#undef TAILQ_ENTRY
#define	TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
	TRACEBUF							\
}

/*
 * Tail queue functions.
 */
#undef TAILQ_CONCAT
#define	TAILQ_CONCAT(head1, head2, field) do {				\
	if (!TAILQ_EMPTY(head2)) {					\
		*(head1)->tqh_last = (head2)->tqh_first;		\
		(head2)->tqh_first->field.tqe_prev = (head1)->tqh_last;	\
		(head1)->tqh_last = (head2)->tqh_last;			\
		TAILQ_INIT((head2));					\
		QMD_TRACE_HEAD(head1);					\
		QMD_TRACE_HEAD(head2);					\
	}								\
} while (0)

#undef TAILQ_EMPTY
#define	TAILQ_EMPTY(head)	((head)->tqh_first == NULL)

#undef TAILQ_FIRST
#define	TAILQ_FIRST(head)	((head)->tqh_first)

#undef TAILQ_FOREACH
#define	TAILQ_FOREACH(var, head, field)					\
	for ((var) = TAILQ_FIRST((head));				\
	    (var);							\
	    (var) = TAILQ_NEXT((var), field))

#undef TAILQ_FOREACH_FROM
#define	TAILQ_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : TAILQ_FIRST((head)));		\
	    (var);							\
	    (var) = TAILQ_NEXT((var), field))

#undef TAILQ_FOREACH_SAFE
#define	TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = TAILQ_FIRST((head));				\
	    (var) && ((tvar) = TAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#undef TAILQ_FOREACH_FROM_SAFE
#define	TAILQ_FOREACH_FROM_SAFE(var, head, field, tvar)			\
	for ((var) = ((var) ? (var) : TAILQ_FIRST((head)));		\
	    (var) && ((tvar) = TAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#undef TAILQ_FOREACH_REVERSE
#define	TAILQ_FOREACH_REVERSE(var, head, headname, field)		\
	for ((var) = TAILQ_LAST((head), headname);			\
	    (var);							\
	    (var) = TAILQ_PREV((var), headname, field))

#undef TAILQ_FOREACH_REVERSE_FROM
#define	TAILQ_FOREACH_REVERSE_FROM(var, head, headname, field)		\
	for ((var) = ((var) ? (var) : TAILQ_LAST((head), headname));	\
	    (var);							\
	    (var) = TAILQ_PREV((var), headname, field))

#undef TAILQ_FOREACH_REVERSE_SAFE
#define	TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)	\
	for ((var) = TAILQ_LAST((head), headname);			\
	    (var) && ((tvar) = TAILQ_PREV((var), headname, field), 1);	\
	    (var) = (tvar))

#undef TAILQ_FOREACH_REVERSE_FROM_SAFE
#define	TAILQ_FOREACH_REVERSE_FROM_SAFE(var, head, headname, field, tvar) \
	for ((var) = ((var) ? (var) : TAILQ_LAST((head), headname));	\
	    (var) && ((tvar) = TAILQ_PREV((var), headname, field), 1);	\
	    (var) = (tvar))

#undef TAILQ_INIT
#define	TAILQ_INIT(head) do {						\
	TAILQ_FIRST((head)) = NULL;					\
	(head)->tqh_last = &TAILQ_FIRST((head));			\
	QMD_TRACE_HEAD(head);						\
} while (0)

#undef TAILQ_INSERT_AFTER
#define	TAILQ_INSERT_AFTER(head, listelm, elm, field) do {		\
	if ((TAILQ_NEXT((elm), field) = TAILQ_NEXT((listelm), field)) != NULL)\
		TAILQ_NEXT((elm), field)->field.tqe_prev = 		\
		    &TAILQ_NEXT((elm), field);				\
	else {								\
		(head)->tqh_last = &TAILQ_NEXT((elm), field);		\
		QMD_TRACE_HEAD(head);					\
	}								\
	TAILQ_NEXT((listelm), field) = (elm);				\
	(elm)->field.tqe_prev = &TAILQ_NEXT((listelm), field);		\
	QMD_TRACE_ELEM(&(elm)->field);					\
	QMD_TRACE_ELEM(&listelm->field);				\
} while (0)

#undef TAILQ_INSERT_BEFORE
#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	TAILQ_NEXT((elm), field) = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &TAILQ_NEXT((elm), field);		\
	QMD_TRACE_ELEM(&(elm)->field);					\
	QMD_TRACE_ELEM(&listelm->field);				\
} while (0)

#undef TAILQ_INSERT_HEAD
#define	TAILQ_INSERT_HEAD(head, elm, field) do {			\
	if ((TAILQ_NEXT((elm), field) = TAILQ_FIRST((head))) != NULL)	\
		TAILQ_FIRST((head))->field.tqe_prev =			\
		    &TAILQ_NEXT((elm), field);				\
	else								\
		(head)->tqh_last = &TAILQ_NEXT((elm), field);		\
	TAILQ_FIRST((head)) = (elm);					\
	(elm)->field.tqe_prev = &TAILQ_FIRST((head));			\
	QMD_TRACE_HEAD(head);						\
	QMD_TRACE_ELEM(&(elm)->field);					\
} while (0)

#undef TAILQ_INSERT_TAIL
#define	TAILQ_INSERT_TAIL(head, elm, field) do {			\
	TAILQ_NEXT((elm), field) = NULL;				\
	(elm)->field.tqe_prev = (head)->tqh_last;			\
	*(head)->tqh_last = (elm);					\
	(head)->tqh_last = &TAILQ_NEXT((elm), field);			\
	QMD_TRACE_HEAD(head);						\
	QMD_TRACE_ELEM(&(elm)->field);					\
} while (0)

#undef TAILQ_LAST
#define	TAILQ_LAST(head, headname)					\
	(*(((struct headname *)((head)->tqh_last))->tqh_last))

#undef TAILQ_NEXT
#define	TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#undef TAILQ_PREV
#define	TAILQ_PREV(elm, headname, field)				\
	(*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))

#undef TAILQ_REMOVE
#define	TAILQ_REMOVE(head, elm, field) do {				\
	QMD_SAVELINK(oldnext, (elm)->field.tqe_next);			\
	QMD_SAVELINK(oldprev, (elm)->field.tqe_prev);			\
	if ((TAILQ_NEXT((elm), field)) != NULL)				\
		TAILQ_NEXT((elm), field)->field.tqe_prev = 		\
		    (elm)->field.tqe_prev;				\
	else {								\
		(head)->tqh_last = (elm)->field.tqe_prev;		\
		QMD_TRACE_HEAD(head);					\
	}								\
	*(elm)->field.tqe_prev = TAILQ_NEXT((elm), field);		\
	TRASHIT(*oldnext);						\
	TRASHIT(*oldprev);						\
	QMD_TRACE_ELEM(&(elm)->field);					\
} while (0)

#undef TAILQ_SWAP
#define TAILQ_SWAP(head1, head2, type, field) do {			\
	struct type *swap_first = (head1)->tqh_first;			\
	struct type **swap_last = (head1)->tqh_last;			\
	(head1)->tqh_first = (head2)->tqh_first;			\
	(head1)->tqh_last = (head2)->tqh_last;				\
	(head2)->tqh_first = swap_first;				\
	(head2)->tqh_last = swap_last;					\
	if ((swap_first = (head1)->tqh_first) != NULL)			\
		swap_first->field.tqe_prev = &(head1)->tqh_first;	\
	else								\
		(head1)->tqh_last = &(head1)->tqh_first;		\
	if ((swap_first = (head2)->tqh_first) != NULL)			\
		swap_first->field.tqe_prev = &(head2)->tqh_first;	\
	else								\
		(head2)->tqh_last = &(head2)->tqh_first;		\
} while (0)

#if 0
/*
 * Headless Tail queue definitions.
 */
#undef HLTQ_ENTRY
#define HLTQ_ENTRY(type)		TAILQ_ENTRY(type)

#undef	HLTQ_INIT
#define	HLTQ_INIT(entry, field) do {					\
	(entry)->field.tqe_next = NULL;					\
	(entry)->field.tqe_prev = &(entry)->field.tqe_next;		\
} while (0)

#undef HLTQ_INITIALIZER
#define HLTQ_INITIALIZER(entry, field)				\
	{ NULL, &(entry)->field.tqe_next }

#undef	HLTQ_FIRST
#define	HLTQ_FIRST(elm)		(elm)

#undef	HLTQ_END
#define	HLTQ_END(elm)		NULL

#undef	HLTQ_NEXT
#define	HLTQ_NEXT(elm, field)	((elm)->field.tqe_next)

#undef HLTQ_LAST
#define HLTQ_LAST(elm, type, field)					\
	((elm)->field.tqe_next == NULL ? (elm) :			\
	    __containerof((elm)->field.tqe_prev, struct type, field.tqe_next))

#undef HLTQ_PREV
#define HLTQ_PREV(elm, type, field)					\
	(*(elm)->field.tqe_prev == NULL ? NULL :			\
	    __containerof((elm)->field.tqe_prev, struct type, field.tqe_next))

#undef HLTQ_FOREACH
#define HLTQ_FOREACH(var, head, field)					\
	for ((var) = HLTQ_FIRST(head);					\
	    (var) != HLTQ_END(head);					\
	    (var) = HLTQ_NEXT(var, field))

#undef	HLTQ_FOREACH_SAFE
#define	HLTQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = HLTQ_FIRST(head);					\
	    (var) != HLTQ_END(head) &&					\
	    ((tvar) = HLTQ_NEXT(var, field), 1);			\
	    (var) = (tvar))

#undef	HLTQ_FOREACH_REVERSE
#define HLTQ_FOREACH_REVERSE(var, head, headname, field)		\
	for ((var) = HLTQ_LAST(head, headname);				\
	    (var) != HLTQ_END(head);					\
	    (var) = HLTQ_PREV(var, headname, field))

#undef	HLTQ_FOREACH_REVERSE_SAFE
#define	HLTQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)	\
	for ((var) = HLTQ_LAST(head, headname);				\
	    (var) != HLTQ_END(head) &&					\
	    ((tvar) = HLTQ_PREV(var, headname, field), 1);		\
	    (var) = (tvar))

/* Concatenate queue2 to the end of queue1. */
#undef HLTQ_CONCAT
#define HLTQ_CONCAT(queue1, queue2, field) do {				\
	(queue2)->field.tqe_prev = (queue1)->field.tqe_prev;		\
	*(queue1)->field.tqe_prev = (queue2);				\
	(queue1)->field.tqe_prev = &(queue2)->field.tqe_next;		\
} while (0)

/* Convert a headless tailq to a headful one. */
#define HLTQ_TO_TAILQ(head, hl, field) do {				\
	(head)->tqh_first = (hl);					\
	(head)->tqh_last = (hl)->field.tqe_prev;			\
	(hl)->field.tqe_prev = &(head)->tqh_first;			\
} while (0)

/* Concatenate a headless tail queue to the end of a regular tail queue. */
#define TAILQ_CONCAT_HLTQ(head, hl, field) do {				\
	void *last = (hl)->field.tqe_prev;				\
	(hl)->field.tqe_prev = (head)->tqh_last;			\
	*(head)->tqh_last = (hl);					\
	(head)->tqh_last = last;					\
} while (0)
#endif

/* 송신 큐 엔트리 */
typedef struct CondorNetIfTxQueueEntry
{
	/* 송신관련 부가 정보 */
	txMetaData_t			meta;

	/* 송신 패킷 */
	struct sk_buff			*skb;
	//BYTE					*pkt;
	//dataUnitLenRange		pktLen;

	/* 직전 및 직후 엔트리를 가리키는 포인터 */
	TAILQ_ENTRY(CondorNetIfTxQueueEntry/*type*/)	entries/*field*/;
} condorNetIfTxQueueEntry_t;

/* 송신 큐 */
TAILQ_HEAD(CondorNetIfTxQueueEntriesHead/*headname*/, CondorNetIfTxQueueEntry/*type*/);
typedef struct
{
	/* 이 큐가 속한 네트워크인터페이스 및 큐의 번호 - 초기화 시에 설정된다.
	 * 	qIndex는 동일 PHY에 할당된 8개 버퍼 사이에서의 번호를 나타낸다. */
    netIfIndexRange			netIfIndex;
	netIfTxQueueIndexRange	qIndex;

	/* 큐에 삽입 가능한 패킷의 최대 개수
	 * 	- 큐 내의 패킷 개수가 이 값에 도달하면, 더 이상 패킷을 삽입할 수 없다.
	 * 	- 큐 삽입을 시도하면 에러를 반환한다. */
	uint32_t		maxPktNum;

	/* 큐의 "almost full" 발생 임계점
	 * 	- 큐에 패킷 삽입 시, 큐 내의 패킷 개수가 이 값에 도달하면, 패킷 삽입 후 almostFull을 반환한다.
	 * 	  almostFull이 반환되면, 상위계층 네트워크 큐를 중지함으로써,IP 패킷의 전달을 막는다.
	 * 	  반면, WSM 패킷은 Full될 때까지 삽입 가능하다. */
	uint32_t		almostFullPktNum;

	/* 큐의 "almost empty" 발생 임계점
	 * 	- 큐에서 패킷을 꺼내 전송 시, 큐 내의 패킷 개수가 이 값에 도달하면, 상위계층 네트워크 큐를 재개하여,
	 * 		IP 패킷이 다시 전달되도록 한다. */
	uint32_t		almostEmptyPktNum;

	/* 현재 큐 내에 존재하는 패킷 개수 */
	uint32_t		pktNum;

	/* Queue Full이 발생한 회수 */
	uint32_t		queueFullCnt;

	/* 이 큐에 연결된 H/W 큐가 처리 중인 패킷 개수
	 * 	- 1-depth H/W 큐 시스템에서는 1일 경우 H/W busy, 0일 경우 H/W idle로 판단한다.
	 * 	- n-depth H/W 큐 시스템에서는 이 값을 보고 추가로 H/W 큐에 삽입할 수 있는 패킷 수(n - hwQueuePktNum)를 확인할 수 있다. */
	uint32_t		hwQueuePktNum;

	/* 큐 내에 삽입된 첫 번째 엔트리와 마지막 엔트리를 가리키는 포인터 */
	struct CondorNetIfTxQueueEntriesHead	head;
} condorNetIfTxQueue_t;

typedef struct CondorRxQueueEntry
{
	/* 수신 관련 부가정보 */
	rxMetaData_t		meta;

	/* 수신 패킷이 저장되는 소켓 버퍼 */
	struct sk_buff		*skb;

	/* 직전 및 직후 엔트리를 가리키는 포인터 */
	TAILQ_ENTRY(CondorRxQueueEntry/*type*/)	entries/*field*/;
} condorRxQueueEntry_t;

TAILQ_HEAD(CondorRxQueueHead/*headname*/, CondorRxQueueEntry/*type*/);
typedef struct CondorRxQueue
{
	/* 큐 내에 삽입할 수 있는 패킷의 최대 개수 */
	uint32_t				maxPktNum;

	/* 현재 큐 내에 삽입된 패킷 개수 */
	uint32_t				pktNum;

	/* 큐 내에 삽입된 첫 번째 엔트리와 마지막 엔트리를 가리키는 포인터 */
	struct CondorRxQueueHead	head;
} condorRxQueue_t;

#endif //LIBCONDOR_CONDOR_QUEUE_H
