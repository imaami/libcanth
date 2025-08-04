#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "b25.h"
#include "b26.h"

#define foreach_prop(it, tab)             \
  for (struct prop *o_ = (it), *it_ = o_; \
       it_; (it) = next_prop((tab), it_), \
             it_ = (it) != o_ ? (it) : nullptr)

struct prop {
	char *name;
};

struct ptab {
	uint32_t    m;
	struct prop p[32];
};

struct perm {
	u16bit rot :  5;
	u16bit seq : 11;
};

static force_inline size_t
mapping (struct ptab *tab,
         unsigned     idx)
{
	return (size_t)(ror32(tab->m, idx) & UINT32_C(31));
}

static struct prop *
property (struct ptab *tab,
          unsigned     idx)
{
	return &tab->p[mapping(tab, idx)];
}

static force_inline size_t
next_idx (struct ptab *tab,
          struct prop *prop)
{
	ptrdiff_t i = (ptrdiff_t)(prop - &tab->p[0]);
	return mapping(tab, (unsigned)i);
}

static struct prop *
next_prop (struct ptab *tab,
           struct prop *prop)
{
	return property(tab, next_idx(tab, prop));
}

static const_inline struct perm
perm (uint16_t id)
{
	return (struct perm){
		.rot = id & UINT16_C(0x01f),
		.seq = id >> 5U,
	};
}

static const_inline uint32_t
perm_map (struct perm p)
{
	return ror32(B25[p.seq], p.rot);
}

static const_inline struct ptab
ptab (uint16_t id)
{
	return (struct ptab){
		.m = perm_map(perm(id))
	};
}

static int alt_main(int c, char **v);

int main (int    argc,
          char **argv)
{
	return alt_main(argc, argv);
	struct ptab props = ptab(70);

	struct prop *p = property(&props, 0);
	int k = 1;
	foreach_prop (p, &props) {
		if (k >= argc)
			break;
		p->name = argv[k++];
	}

	p = property(&props, 0);
	k = 0;
	foreach_prop (p, &props) {
		if (!p->name)
			break;
		printf(&", \"%s\""[!k++ << 1], p->name);
	}
	if (k)
		putchar('\n');

	for (size_t i = 0; i < array_size(props.p); ++i) {
		char const *name = props.p[i].name;
		if (name)
			printf("%2zu \"%s\"\n", i, name);
	}
}

struct bag {
	uint32_t    seq;
	uint8_t     head[2];
	uint8_t     iter[2];
	char const *ptr[32];
};

static const uint64_t BAG_DBSEQ[56] = {
	0xb1aeca7cc653ed7d, 0xab1be9cacc6e53ea, 0x66f2b3e8dfca39be, 0xf46a31f2bb34bea3,
	0xdf2a232f8ed10cad, 0x8f95df35ca3eacd1, 0xe57da3e3728faa8d, 0x4fb946bc3be5a8b4,
	0x4538df5884d6477a, 0x18d64dd1fc4ec6be, 0x48b968fb9365937d, 0x1da4971f64cb5be7,
	0x33ea2593a8df45f3, 0xd4d8e09b98fa896e, 0x89daf97df6a723e7, 0xd94eb7c524e2df4e,
	0x459d6f890e5769f0, 0x2a6a7dc567a7dae2, 0x2fa1a89767b69f72, 0xfae5ea8bec99ea27,
	0xebe25cb2717d3ac4, 0xeb14f9db8967d0ac, 0x326c5f5fbeb13936, 0x134c8adf4cd897d5,
	0xd376be5307d8be6a, 0x13077c9a2addf12c, 0x9371fc4d9752df2b, 0xadcc43c9bac5df15,
	0x62e6a27d4d9713f4, 0xf6695c7fd8b9a93f, 0xe1a9d97c7dc4b35c, 0x7e1beb299c6beca7,
	0x1a137594798ca3ed, 0xe9e43ee568b4f95d, 0x691e85cfb48ad646, 0x6393485f3b48a176,
	0x7c4bb4589f2bb35f, 0xec8a7d73a89767c6, 0x12e2b27c8b2e27d4, 0xc17c9a8ba1cd8afa,
	0x2d77dc98adef5365, 0xa35f4c72b7dff731, 0xb517bd235d9fd3ec, 0xc4ef9525a89dea93,
	0xb93ea2a2ca75f04a, 0x7be589d72ea27cab, 0x90d48b7c934d95f0, 0x2b3f289bacfc574d,
	0xea7df5365c2fd137, 0xcaedf1ba567f465b, 0x2b37df32dc7d70c7, 0xeb28dedd322b7e31,
	0x2a277cb764d47df8, 0x074cadf1ec4cb7d4, 0x6d4df2c756f6b931, 0x0001c9f51b52ea3e,
};

static const_inline uint32_t
bag_seq (unsigned i)
{
	if (i > 118U)
		return 0U;

	unsigned y = (i *= 30U) >> 6U, x = i & 63U;
	uint32_t s = (uint32_t)(BAG_DBSEQ[y] >> x);

	if (x > 34U)
		s |= (uint32_t)(BAG_DBSEQ[y + 1U] << (64U - x));

	return ror32((s >> 4U & 0x3fffffeU) | 0x4000001U, s & 0x1fU);
}

static const_inline struct bag
bag (unsigned i)
{
	if (i > 118U)
		return (struct bag){0};
	uint32_t seq = bag_seq(i);
	uint8_t h2 = i < 65U ? 1U
	                     : (i < 104U ? (2U + (i >= 88U))
	                                 : (i < 116U ? (4U + (i >= 110U))
	                                             : (6U + (i >= 117U))));
	return (struct bag){
		.seq = seq,
		.head = {0, h2},
		.iter = {0, h2},
	};
}

static force_inline uint8_t
bag_map (struct bag const *const bag,
         uint32_t const          idx)
{
	return (uint8_t)(ror32(bag->seq, idx) & UINT32_C(31));
}

static force_inline char const **
bag_ptr (struct bag     *bag,
         uint32_t const  idx)
{
	return &bag->ptr[bag_map(bag, idx)];
}

static force_inline uint32_t
bag_idx (struct bag const *const bag,
         char const      **const ptr)
{
	return (uint32_t)(ptrdiff_t)(ptr - &bag->ptr[0]);
}

static force_inline char const **
bag_iter (struct bag *const bag,
          bool const        second)
{
	uint8_t const i = bag->head[second];
	bag->iter[second] = i;
	return &bag->ptr[i];
}

static force_inline void
bag_move_head (struct bag *const bag,
               bool const        second,
               unsigned          amount)
{
	uint8_t i = bag->head[second];
	for (; amount; --amount) {
		i = bag_map(bag, i);
	}
	bag->head[second] = i;
}

static force_inline char const **
bag_next (struct bag *const bag,
          bool const        second)
{
	uint8_t const i = bag_map(bag, bag->iter[second]);
	bag->iter[second] = i;
	return i == bag->head[second] ? nullptr : &bag->ptr[i];
}

static uint64_t
bag_gen (struct bag *const a,
         struct bag *const b)
{
	char const **i0 = bag_iter(a, 0);
	char const **i1 = bag_iter(b, 1);
	uint64_t u64 = 0U;
	size_t bs = 64U;
	do {
		unsigned char const *ucp[] = {
			(unsigned char const *)*i0,
			(unsigned char const *)*i1
		};
		size_t len = ucp[0][0];
		bs -= len;
		len *= ucp[0][1] == (unsigned char)'1';
		u64 |= (UINT64_C(1) << len) - 1U << bs;
		bs -= (len = ucp[1][0]);
		len *= ucp[1][1] == (unsigned char)'1';
		u64 |= (UINT64_C(1) << len) - 1U << bs;
	} while ((i0 = bag_next(a, 0)) &&
	         (i1 = bag_next(b, 1)));

	return u64;
}

static void
bags_scan (struct bag *const     b,
           uint64_t const *const b26,
           unsigned const        y,
           unsigned const        x)
{
	for (unsigned i = 0; i < 119U; ++i) {
		for (unsigned j = i; j < 119U; ++j) {
			for (unsigned r = 0U; r < 16U; ++r, bag_move_head(&b[j], 1, 1)) {
				struct b26_id id = {0};
				uint64_t u64 = b26_identify(bag_gen(&b[i], &b[j]), &id, b26);
				if (u64) {
					printf("a=%3u 0x%08" PRIx32 " b=%3u 0x%08" PRIx32
					       " r=%2u 0x%016" PRIx64 " (%" PRIu32 ",%" PRIu32 ")\n",
					       i, b[i].seq, j, b[j].seq, r, u64,
					       (uint32_t)id.idx, (uint32_t)id.rot);
				}
				if (i == j)
					continue;
				id = (struct b26_id){0};
				u64 = b26_identify(bag_gen(&b[j], &b[i]), &id, b26);
				if (u64) {
					printf("a=%3u 0x%08" PRIx32 " b=%3u 0x%08" PRIx32
					       " r=%2u 0x%016" PRIx64 " (%" PRIu32 ",%" PRIu32 ")\n",
					       j, b[j].seq, i, b[i].seq, r, u64,
					       (uint32_t)id.idx, (uint32_t)id.rot);
				}
			}
			if (i == j)
				continue;
			for (unsigned r = 0U; r < 16U; ++r, bag_move_head(&b[i], 1, 1)) {
				struct b26_id id = {0};
				uint64_t u64 = b26_identify(bag_gen(&b[j], &b[i]), &id, b26);
				if (!u64)
					continue;
				printf("a=%3u 0x%08" PRIx32 " b=%3u 0x%08" PRIx32
				       " r=%2u 0x%016" PRIx64 " (%" PRIu32 ",%" PRIu32 ")\n",
				       j, b[j].seq, i, b[i].seq, r, u64,
				       (uint32_t)id.idx, (uint32_t)id.rot);
			}
		}
	}
}

static int
alt_main (int    c,
          char **v)
{
	uint64_t *b26 = nullptr;
	if (c == 2) {
		if (!(b26 = b26_load(v[1], false)))
			return 1;
	} else for (int i = 0; ++i < c;) {
		uint16_t id = (uint16_t)strtoul(v[i], nullptr, 0);
		struct b25_info info = b25_info_by_id(b25_id(id));
		b25_info_print(&info, b25_id(id));
	}

	if (c > 2)
		return 0;

#if 0
	struct b25_info info[array_size(two16)] = {0};
	for (size_t i = 0; i < array_size(two16); ++i)
		info[i] = b25_info_by_id(two16[i].seq << 5U | two16[i].rot);
	for (size_t i = 0; i < array_size(two16); ++i)
		b25_info_print(&info[i], two16[i].seq << 5U | two16[i].rot);
#endif
	char const *const ptr[] = {
		"\6" "000000", "\4" "0000", "\3" "000", "\3" "000", "\2" "00", "\2" "00", "\2" "00",
		"\2" "00", "\1" "0", "\1" "0", "\1" "0", "\1" "0", "\1" "0", "\1" "0", "\1" "0", "\1" "0",
		"\6" "111111", "\4" "1111", "\3" "111", "\3" "111", "\2" "11", "\2" "11", "\2" "11",
		"\2" "11", "\1" "1", "\1" "1", "\1" "1", "\1" "1", "\1" "1", "\1" "1", "\1" "1", "\1" "1",
	};

	uint32_t mask[119];
	struct bag b[119];

	for (unsigned i = 0; i < 119U; ++i) {
		b[i] = bag(i);
		mask[i] = b25_info(b[i].seq).mask[0];
	}

	for (unsigned y = 0; y < 16; ++y) {
		for (unsigned x = 0; x < 16; ++x) {
			for (unsigned i = 0; i < 119U; ++i) {
				unsigned k0 = x;
				unsigned k1 = y;
				for (uint32_t n = 0, m = mask[i]; n < 32U; ++n, m >>= 1U) {
					b[i].ptr[n] = (m & 1U)
						? ptr[(k0++ & 15U)]
						: ptr[(k1++ & 15U) + 16U];
				}
			}
			bags_scan(b, b26, y, x);
		}
	}

	free(b26);
	return 0;
}
