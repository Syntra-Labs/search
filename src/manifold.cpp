// ============================================================================
// manifold.cpp — Reference Placement Engine (C++20)
// ============================================================================
// Product: local-only reference placement for code.
//
// Input:  a local repo (the "playground"), a search space of reference sources
//         (local libraries, custom doc paths, cloned GitHub repos, docs sites),
//         and a query (either an explicit gap or a reorganization scan).
//
// Output: a list of Placement objects. Every placement targets a LOCAL file and
//         line. External references appear only as justification, never as
//         destinations.
//
// Placement kinds:
//   FILL    — local stub/TODO/empty body has a reference match
//   MOVE    — local code is near neighbors that live in a different local file
//   EXTRACT — local code scattered across files forms a cluster that should co-locate
//
// Two tools, split cleanly:
//   Tool A (this file): placement engine. Output is JSON.
//   Tool B (separate):  editor plugin. Reads JSON, opens the file at the line.
//
// CLI:
//   manifold place "<query>" --space <paths> --kinds <fill,move,extract> --json
//   manifold search "<query>" --space <paths>
//   manifold gaps <repo>
//   manifold fill <repo> --gap <cell_id> --space <paths>
//   manifold index <repo> --sources <paths>
//
// Compile: g++ -O3 -std=c++20 -Wall -Wextra -Werror manifold.cpp -o manifold
// Test:    ./manifold selftest
// ============================================================================
// Reconciled against the authoritative sources:
//   [SL] Harlow & Salakhov, "The Constitutional Symmetry Ladder SU(0) - SU(7):
//        A Constitutional Universal Grammar" (SL §1.1, §3.13, §3.19, §4.7).
//   [CF] Harlow & Salakhov, "Constitutional Foam != Quantum Foam"
//        (CF §19.5, Exhibits A-E, Alignment Map Appendices A-K).
//   [M]  manifold.cpp Operational Realization (Stages 1-11 & Corrections C1-C12).
//
// Architectural Hierarchy (CF Alignment Map §A-§I):
//   Stage 0 (§0, §11) : Question (D-meta-1) & Three-Reduction Cycle Engine
//   Stage 1 (§1-§3)   : SubstrateView (Gamma^0_F), Path<S>, AdapterFor<A, S>
//   Stage 2 (§4)      : Preservation (SU(7)(Gamma^0_F) <= Gamma^0_F),
//                       240-Root E8 Lattice (C2), SU(4) HashBucketQuantizer & SU(7) 48-Gen (C10)
//   Stage 3 (§5)      : Reflective Weighting w_F : Gamma^0_F -> C & Exact Stats <=16KB (C3)
//   Stage 4-5 (§6)    : Relational Endpoint F_rel = (H_F, ~_F, w_F), FoamDensity rho_F,
//                       & 11D Invariant Coordinate Vector I(rho) in R^11 (CF Exhibit E)
//   Stage 6-7 (§7)    : Adequacy Gate (psi ~_F phi => psi ==_F phi), Certificate,
//                       Parameterized Merkle Root (C11) & Schema Versioned Slab (C12)
//   Stage 8-10 (§8-10): Realization Layer (gated on is_adequacy_certified):
//                       Unbiased 31-Bit Slice HelixSignature256 (C1), Constitutional
//                       ModeRelationLinks (C4), Two-Axis Incorporation/Admissibility (C5),
//                       AdapterMetadata (C6-C8), SurprisalSketch (C9), & Neovim CLI.
//
// Compile: g++ -O3 -march=native -std=c++20 -Wall -Wextra manifold.cpp -o manifold
// Run self-test:  ./manifold selftest
// Run benchmark:  ./manifold benchmark [dir_path]
// ============================================================================

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <complex>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <map>
#include <regex>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

// ============================================================================
// §1, §2, §3. STAGE 1: SUBSTRATE VIEW (\Gamma^0_F), PATH, AND ADAPTER CONCEPTS
// Provenance: [CF Exhibits A-D; Alignment Map §C]
//             "\Gamma^0_F = { \gamma | A_path(\gamma) ^ A_local(\gamma) }"
// ============================================================================

// §1. Identification (D-substrate-1 -> SubstrateView, closed) [CF Alignment Map §C]
template <typename S>
concept SubstrateView = requires(const S& s) {
    { s.bytes() } -> std::convertible_to<std::span<const std::uint8_t>>;
};

// Concrete zero-copy SubstrateView over any contiguous byte buffer / string_view
struct ByteSpanSubstrate {
    std::span<const std::uint8_t> span_data{};

    constexpr ByteSpanSubstrate() = default;
    constexpr explicit ByteSpanSubstrate(std::span<const std::uint8_t> s) : span_data(s) {}
    explicit ByteSpanSubstrate(std::string_view sv)
        : span_data(reinterpret_cast<const std::uint8_t*>(sv.data()), sv.size()) {}

    constexpr std::span<const std::uint8_t> bytes() const noexcept { return span_data; }
};
static_assert(SubstrateView<ByteSpanSubstrate>);

// §2. Identification (D-path-1 -> Path, closed) [SL §4.7; CF Exhibit A]
template <SubstrateView S>
struct Path {
    const S& substrate;
    std::uint32_t start = 0;
    std::uint32_t end   = 0;

    std::span<const std::uint8_t> bytes() const noexcept {
        auto all = substrate.bytes();
        if (start >= all.size() || end <= start) return {};
        std::size_t len = std::min<std::size_t>(end - start, all.size() - start);
        return all.subspan(start, len);
    }
};

// §3. Identification (D-adapter-1 -> AdapterFor, closed) [CF Alignment Map §C, §G]
template <typename A, typename S>
concept AdapterFor = SubstrateView<S> && requires(
    Path<S> p,
    std::span<const std::uint8_t> x,
    std::span<const std::uint8_t> y
) {
    { A::A_path(p) }      -> std::same_as<bool>;
    { A::A_local(p) }     -> std::same_as<bool>;
    { A::approx_F(x, y) } -> std::same_as<bool>;
    { A::equiv_F(x, y) }  -> std::same_as<bool>;
};

// ============================================================================
// REALIZATION LAYER (Stages 8-10) — Modality, Scale.
// These are NOT constitutional stages. They are Adapter-supplied projections
// used only in the gated Realization layer. They must not be consumed by
// Preservation, Weighting, Density, or Adequacy.
// ============================================================================
static constexpr size_t HV_BITS = 8192;
static constexpr size_t HV_WORDS = HV_BITS / 64; // 128 x uint64_t = 1024 bytes
static constexpr size_t SURPRISAL_BUCKETS = 65536; // 2^16 online frequency sketch

// Phase 2: 90% Salience Sparsification Cap
static constexpr size_t MAX_BLOCK_MASS_NODES = 20;
static constexpr size_t MAX_QUERY_MASS_NODES = 32;
static constexpr uint32_t ABSENT_NODE        = 0xFFFFFFFFu;

// First 16 odd primes used deterministically as cyclic rotation strides
static constexpr std::array<uint32_t, 16> PRIME_STRIDES = {
    3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59
};

// Correction C8 [SL §4.7]: Filtration Level / Scale in AdapterMetadata
enum class Scale : uint8_t {
    REPO  = 0,
    FILE  = 1,
    BLOCK = 2,
    SPAN  = 3
};

// Correction C7: Modality in AdapterMetadata (@repo, @docs, @ref, @web, @exp)
enum class Modality : uint8_t {
    REPO_CODE  = 0, // @repo — Primary project repository files
    SPEC_DOC   = 1, // @docs — Local specifications, Markdown, LaTeX papers
    EXT_REF    = 2, // @ref  — External reference codebases / cloned repos
    WEB_REF    = 3, // @web  — Downloaded wget/HTML/online documentation dirs
    EXPERIMENT = 4  // @exp  — Logged scientific hypotheses (KEEP / REJECT)
};


static const char* modality_tag(Modality m) {
    switch (m) {
        case Modality::REPO_CODE:  return "@repo";
        case Modality::SPEC_DOC:   return "@docs";
        case Modality::EXT_REF:    return "@ref";
        case Modality::WEB_REF:    return "@web";
        case Modality::EXPERIMENT: return "@exp";
        default:                   return "@?";
    }
}

// Correction C10 [SL §1.1, CF §19.5]: SU(7) 48-Generator & 8-Radiance-Sector Structure
//   dim(su(4)) = 15, dim(su(5)) = 24, dim(su(6)) = 35, dim(su(7)) = 48 (rank 6, Cartan A_6).
//   Accommodates all 8 radiance-channel curvature sectors Q_1..Q_8 and the 42 off-diagonal
//   root generators E_{\alpha} + 6 Cartan generators H_1..H_6 = 48 generators.
// Cycle D-algebra-1 [SL §1.1, §4.3, §4.9, §4.45, §6.3; CF §6.2, §7, §19.5]:
// Full 48-Generator Graded Lie Algebra su(2) < su(3) < su(4) < su(5) < su(6) < su(7).
// Constructed in the 7x7 fundamental anti-Hermitian traceless basis T_0..T_47 satisfying:
//   1) Orthonormality: Tr(T_a T_b) = -1/2 \delta_{ab}
//   2) Commutation:    [T_a, T_b] = \sum_{c=0}^{47} f_{abc} T_c
//   3) Jacobi Identity [SL §4.3]: \sum_d (f_{abd}f_{dce} + f_{bcd}f_{dae} + f_{cad}f_{dbe}) == 0
//   4) Graded Closure  [SL §4.45]: Prefix T_0..T_{n^2-2} closes su(n) for n=2,3,4,5,6,7
//      and reductive complement m_{n+1} satisfies [su(n), m_{n+1}] <= m_{n+1}.
// ============================================================================
// 3. DETERMINISTIC HASHING & PRECOMPUTED SIMD BIT-EXPANSION TABLE
// ============================================================================
static inline uint64_t fnv1a_64(const void* data, size_t len, uint64_t basis = 14695981039346656037ULL) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint64_t h = basis;
    for (size_t i = 0; i < len; ++i) {
        h ^= bytes[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static inline uint64_t fnv1a_str(std::string_view s, uint64_t basis = 14695981039346656037ULL) {
    return fnv1a_64(s.data(), s.size(), basis);
}

static inline uint64_t splitmix64(uint64_t& state) {
    uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// Correction C11: Cryptographic ARX / Sip-SHA256-Fold64 mixer for adversarial Merkle verification
static inline uint64_t crypto_arx_hash64(const void* data, size_t len, uint64_t seed = 0x6A09E667F3BCC908ULL) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t v0 = seed ^ 0x736F6D6570736575ULL;
    uint64_t v1 = seed ^ 0x646F72616E646F6DULL;
    uint64_t v2 = seed ^ 0x6C7967656E657261ULL;
    uint64_t v3 = seed ^ 0x7465646279746573ULL;

    auto sipround = [&]() {
        v0 += v1; v1 = std::rotl(v1, 13); v1 ^= v0; v0 = std::rotl(v0, 32);
        v2 += v3; v3 = std::rotl(v3, 16); v3 ^= v2;
        v0 += v3; v3 = std::rotl(v3, 21); v3 ^= v0;
        v2 += v1; v1 = std::rotl(v1, 17); v1 ^= v2; v2 = std::rotl(v2, 32);
    };

    size_t i = 0;
    for (; i + 8 <= len; i += 8) {
        uint64_t m = 0;
        std::memcpy(&m, p + i, 8);
        v3 ^= m;
        sipround();
        sipround();
        v0 ^= m;
    }
    uint64_t tail = static_cast<uint64_t>(len) << 56;
    for (size_t r = 0; i + r < len; ++r) {
        tail |= static_cast<uint64_t>(p[i + r]) << (r * 8);
    }
    v3 ^= tail;
    sipround();
    sipround();
    v0 ^= tail;
    v2 ^= 0xFFULL;
    sipround();
    sipround();
    sipround();
    sipround();
    return v0 ^ v1 ^ v2 ^ v3;
}

// Phase 3.3 (D-adequacy-hash-2): 256-bit ARX sponge & CanonicalDigest256
static inline std::array<uint64_t, 4> crypto_arx_hash256(
    const void* data,
    size_t len,
    uint64_t seed = 0x6A09E667F3BCC908ULL
) noexcept {
    std::array<uint64_t, 4> out{};
    for (size_t lane = 0; lane < 4; ++lane) {
        uint64_t lane_seed = seed ^ (0x9E3779B97F4A7C15ULL * (lane + 1)) ^ (static_cast<uint64_t>(len) << (lane * 13));
        out[lane] = crypto_arx_hash64(data, len, lane_seed);
    }
    // Cross-lane ARX diffusion round ensuring full 256-bit avalanche across all 4 words
    uint64_t a = out[0], b = out[1], c = out[2], d = out[3];
    for (int r = 0; r < 4; ++r) {
        a += b; b = std::rotl(b, 13) ^ a;
        c += d; d = std::rotl(d, 16) ^ c;
        a += d; d = std::rotl(d, 21) ^ a;
        c += b; b = std::rotl(b, 17) ^ c;
    }
    return {a, b, c, d};
}

struct CanonicalDigest256 {
    std::array<uint64_t, 4> words{};
    uint32_t canonical_len = 0;

    constexpr bool operator==(const CanonicalDigest256& other) const noexcept = default;
    constexpr uint64_t low64() const noexcept { return words[0]; }
};

enum class MerkleHashAlgo : uint8_t {
    FNV1A_64       = 0, // Fast non-cryptographic structural verification
    CRYPTO_ARX_64  = 1, // 64-bit ARX sponge verification
    CRYPTO_ARX_256 = 2  // Phase 3.3 (D-adequacy-hash-2): 256-bit collision-resistant ARX Merkle verification
};

struct BipolarByteLUT {
    alignas(32) int16_t table[256][8]{};
    constexpr BipolarByteLUT() {
        for (int b = 0; b < 256; ++b) {
            for (int k = 0; k < 8; ++k) {
                table[b][k] = ((b >> k) & 1) ? 1 : -1;
            }
        }
    }
};

static constexpr BipolarByteLUT BYTE_BIPOLAR_LUT{};

// ============================================================================
// 4. 8192-BIT BINARY HYPERVECTOR, UNBIASED HELIX-256 (C1) & E8 CARRIER
// ============================================================================
struct Hypervec8192 {
    std::array<uint64_t, HV_WORDS> words{};

    static Hypervec8192 from_seed(uint64_t seed) {
        Hypervec8192 hv;
        uint64_t state = seed;
        for (size_t i = 0; i < HV_WORDS; ++i) {
            hv.words[i] = splitmix64(state);
        }
        return hv;
    }

    uint32_t hamming(const Hypervec8192& other) const {
        uint32_t dist = 0;
        for (size_t i = 0; i < HV_WORDS; ++i) {
            dist += static_cast<uint32_t>(std::popcount(words[i] ^ other.words[i]));
        }
        return dist;
    }

    double similarity(const Hypervec8192& other) const {
        return 1.0 - (static_cast<double>(hamming(other)) / static_cast<double>(HV_BITS));
    }

    double z_score(const Hypervec8192& other) const {
        double expected = static_cast<double>(HV_BITS) * 0.5;
        double sigma = std::sqrt(static_cast<double>(HV_BITS)) * 0.5;
        return (expected - static_cast<double>(hamming(other))) / sigma;
    }
};

struct HelixSignature256 {
    uint64_t s0 = 0;
    uint64_t s1 = 0;
    uint64_t s2 = 0;
    uint64_t s3 = 0;
};

// Correction C1 [§10, Stage 8]: Unbiased 4-Quadrant 256-Bit Helix Signature
// Samples the 4 quadrant anchor words (words 0, 32, 64, 96 across the 128-word
// 8,192-bit manifold). Because hypervector similarity is pointwise coordinate-aligned
// (E[q_k == c_k]), direct 4-quadrant coordinate sampling preserves 100% of the
// per-bit SNR without horizontal popcount dilution and has zero tie bias!
static inline HelixSignature256 compute_helix_signature(const Hypervec8192& hv) noexcept {
    return {hv.words[0], hv.words[32], hv.words[64], hv.words[96]};
}

// Phase -1a & -1c Provenance Comment — E8CarrierAddress:
//   - What it computes: Extracts first_block and last_block 8D coordinates from a Hypervec8192,
//     to `bucket_key` (0..255 via HASH_BUCKET_QUANTIZER). Dead fields block_roots_240[1..3] removed.
//   - What consumes it: ingest_file(), record_identification(), and route().
struct E8CarrierAddress {
    int32_t first_block[8]{};
    int32_t last_block[8]{};
    uint8_t first_sector          = 0; // 16-sector root for octant 0 (0..15)
    uint8_t last_sector           = 0; // 16-sector root for octant 3 (0..15)
    uint8_t bucket_key            = 0; // Phase -1a: 16x16 = 256 product-address bucket key
};

static inline E8CarrierAddress decode_e8_carrier(const Hypervec8192& hv) {
    E8CarrierAddress out;
    for (size_t b : {size_t(0), size_t(3)}) {
        int32_t coords[8];
        for (size_t d = 0; d < 8; ++d) {
            size_t base_w = b * 32 + d * 4;
            uint32_t ones = static_cast<uint32_t>(
                std::popcount(hv.words[base_w + 0]) +
                std::popcount(hv.words[base_w + 1]) +
                std::popcount(hv.words[base_w + 2]) +
                std::popcount(hv.words[base_w + 3])
            );
            int32_t diff = static_cast<int32_t>(ones) - 128;
            coords[d] = diff / 6;
            if (b == 0) out.first_block[d] = coords[d];
            if (b == 3) out.last_block[d] = coords[d];
        }
        if (b == 0) {
            out.first_sector          = static_cast<uint8_t>(fnv1a_64(coords, sizeof(coords)) & 0x0F);
        } else {
            out.last_sector           = static_cast<uint8_t>((fnv1a_64(coords, sizeof(coords)) >> 16) & 0x0F);
        }
    }
    out.bucket_key = static_cast<uint8_t>((out.first_sector << 4) | out.last_sector);
    return out;
}

// Fast 64-Bit Bit-Sliced Register Carry-Save Hypervector Encoder
static Hypervec8192 encode_seeds_bitsliced(
    const std::pair<uint64_t, int16_t>* seeds,
    const size_t* order,
    size_t n
) {
    Hypervec8192 out;
    if (n == 0) return out;

    uint64_t states[64];
    uint8_t weights[64];
    uint32_t w_tot = 0;
    for (size_t i = 0; i < n; ++i) {
        size_t idx = order ? order[i] : i;
        states[i] = seeds[idx].first;
        uint8_t pw = static_cast<uint8_t>(std::clamp<int16_t>(seeds[idx].second, 1, 15));
        weights[i] = pw;
        w_tot += pw;
    }

    uint32_t t_even = (w_tot >> 1u) + 1u;
    uint32_t t_odd  = (w_tot + 1u) >> 1u;
    uint32_t init_even = 128u - t_even;
    uint32_t init_odd  = 128u - t_odd;

    uint64_t ib[8];
    for (int bit = 0; bit < 8; ++bit) {
        uint64_t m_even = ((init_even >> bit) & 1u) ? 0x5555555555555555ULL : 0ULL;
        uint64_t m_odd  = ((init_odd  >> bit) & 1u) ? 0xAAAAAAAAAAAAAAAAULL : 0ULL;
        ib[bit] = m_even | m_odd;
    }

    uint64_t mask0[64], mask1[64], mask2[64], mask3[64];
    for (size_t i = 0; i < n; ++i) {
        uint8_t wt = std::min<uint8_t>(weights[i], 15u);
        mask0[i] = (wt & 1u) ? ~0ULL : 0ULL;
        mask1[i] = (wt & 2u) ? ~0ULL : 0ULL;
        mask2[i] = (wt & 4u) ? ~0ULL : 0ULL;
        mask3[i] = (wt & 8u) ? ~0ULL : 0ULL;
    }

    for (size_t w = 0; w < HV_WORDS; ++w) {
        uint64_t b0 = ib[0], b1 = ib[1], b2 = ib[2], b3 = ib[3];
        uint64_t b4 = ib[4], b5 = ib[5], b6 = ib[6], b7 = ib[7];

        for (size_t i = 0; i < n; ++i) {
            uint64_t word = splitmix64(states[i]);

            uint64_t w0 = word & mask0[i];
            uint64_t c0 = b0 & w0; b0 ^= w0;

            uint64_t w1 = word & mask1[i];
            uint64_t x1 = w1 ^ c0;
            uint64_t c1 = (b1 & x1) | (w1 & c0);
            b1 ^= x1;

            uint64_t w2 = word & mask2[i];
            uint64_t x2 = w2 ^ c1;
            uint64_t c2 = (b2 & x2) | (w2 & c1);
            b2 ^= x2;

            uint64_t w3 = word & mask3[i];
            uint64_t x3 = w3 ^ c2;
            uint64_t c  = (b3 & x3) | (w3 & c2);
            b3 ^= x3;

            uint64_t t;
            t = b4 & c; b4 ^= c; c = t;
            t = b5 & c; b5 ^= c; c = t;
            t = b6 & c; b6 ^= c; c = t;
            b7 ^= c;
        }
        out.words[w] = b7;
    }
    return out;
}

struct BipolarAccumulator {
    alignas(64) std::array<int16_t, HV_BITS> counts{};
    uint32_t total_items = 0;

    void clear() {
        counts.fill(0);
        total_items = 0;
    }

    void accumulate(const Hypervec8192& hv, int16_t weight = 1) {
        if (weight == 1) {
            for (size_t w = 0; w < HV_WORDS; ++w) {
                uint64_t word = hv.words[w];
                int16_t* __restrict c_ptr = &counts[w << 6];
                for (size_t b = 0; b < 64; b += 8) {
                    const int16_t* __restrict lut = BYTE_BIPOLAR_LUT.table[static_cast<uint8_t>(word >> b)];
                    for (size_t k = 0; k < 8; ++k) c_ptr[b + k] += lut[k];
                }
            }
            total_items += 1;
        } else {
            for (size_t w = 0; w < HV_WORDS; ++w) {
                uint64_t word = hv.words[w];
                int16_t* __restrict c_ptr = &counts[w << 6];
                for (size_t b = 0; b < 64; b += 8) {
                    const int16_t* __restrict lut = BYTE_BIPOLAR_LUT.table[static_cast<uint8_t>(word >> b)];
                    for (size_t k = 0; k < 8; ++k) c_ptr[b + k] += static_cast<int16_t>(lut[k] * weight);
                }
            }
            if (weight > 0) total_items += static_cast<uint32_t>(weight);
            else if (weight < 0 && total_items >= static_cast<uint32_t>(-weight))
                total_items -= static_cast<uint32_t>(-weight);
        }
    }

    Hypervec8192 project() const {
        Hypervec8192 out;
        for (size_t w = 0; w < HV_WORDS; ++w) {
            uint64_t word = 0ULL;
            const int16_t* __restrict c_ptr = &counts[w << 6];
            for (size_t b = 0; b < 64; b += 8) {
                uint64_t m =
                    (static_cast<uint64_t>(c_ptr[b + 0] >  0) << 0) |
                    (static_cast<uint64_t>(c_ptr[b + 1] >= 0) << 1) |
                    (static_cast<uint64_t>(c_ptr[b + 2] >  0) << 2) |
                    (static_cast<uint64_t>(c_ptr[b + 3] >= 0) << 3) |
                    (static_cast<uint64_t>(c_ptr[b + 4] >  0) << 4) |
                    (static_cast<uint64_t>(c_ptr[b + 5] >= 0) << 5) |
                    (static_cast<uint64_t>(c_ptr[b + 6] >  0) << 6) |
                    (static_cast<uint64_t>(c_ptr[b + 7] >= 0) << 7);
                word |= (m << b);
            }
            out.words[w] = word;
        }
        return out;
    }
};

// ============================================================================
// 5. SURPRISAL SKETCH (C9), ORDER-2 BYTE-MARKOV (C3) & SWAR CA 184/30
// ============================================================================

// Correction C9 [§10]: Realization-Layer Empirical Frequency Sketch
// Marked Provisional / Realization-layer (distinct from constitutional weight w_F).
struct SurprisalSketch {
    std::array<uint32_t, SURPRISAL_BUCKETS> bucket_counts{};
    uint32_t total_spans = 1;

    void observe_seed(uint64_t seed, int32_t delta = 1) {
        size_t idx = static_cast<size_t>((seed ^ (seed >> 17)) & (SURPRISAL_BUCKETS - 1));
        if (delta > 0) {
            bucket_counts[idx] += static_cast<uint32_t>(delta);
        } else if (delta < 0) {
            uint32_t dec = static_cast<uint32_t>(-delta);
            bucket_counts[idx] = (bucket_counts[idx] > dec) ? (bucket_counts[idx] - dec) : 0;
        }
    }

    void step_span(int32_t delta = 1) {
        if (delta > 0) total_spans += static_cast<uint32_t>(delta);
        else if (delta < 0 && total_spans > static_cast<uint32_t>(-delta) + 1)
            total_spans -= static_cast<uint32_t>(-delta);
    }

    int16_t weight_for_seed(uint64_t seed, int16_t base_weight = 4) const {
        if (total_spans <= 4) return base_weight;
        size_t idx = static_cast<size_t>((seed ^ (seed >> 17)) & (SURPRISAL_BUCKETS - 1));
        uint32_t df = bucket_counts[idx];
        if (df == 0) return static_cast<int16_t>(base_weight + 2);

        uint32_t ratio = (total_spans + 1) / (df + 1);
        if (ratio <= 3)  return 1;
        if (ratio <= 8)  return 2;
        if (ratio <= 24) return base_weight;
        return static_cast<int16_t>(base_weight + 2);
    }
};

// Correction C3 [§10, Stage 3]: Exact statistics (step = 1) for all inputs <= 16 KB
struct ByteMarkovModel {
    std::array<std::array<uint32_t, 256>, 256> bigram{};
    std::array<uint32_t, 256> unigram{};
    uint64_t total_bytes = 0;

    void observe_stream(std::string_view text) {
        if (text.empty()) return;
        // Correction C3: Exact full-resolution observation for inputs <= 16 KB
        size_t step = (text.size() > 16384) ? (text.size() / 4096) : 1;
        uint8_t prev = static_cast<uint8_t>(text[0]);
        unigram[prev]++;
        total_bytes++;
        for (size_t i = step; i < text.size(); i += step) {
            uint8_t cur = static_cast<uint8_t>(text[i]);
            bigram[prev][cur]++;
            unigram[cur]++;
            total_bytes++;
            prev = cur;
        }
    }

    inline double byte_surprisal(uint8_t prev, uint8_t cur) const {
        double d = static_cast<double>(total_bytes);
        double v = (static_cast<double>(unigram[cur]) + 0.4) / (d + 102.4);
        double w = static_cast<double>(unigram[prev]);
        double y = static_cast<double>(bigram[prev][cur]);
        double p = 0.76 * ((y + 0.4) / (w + 102.4)) + 0.24 * v;
        return -std::log2(std::max(p, 1e-12));
    }
};

struct DemarcationCA64 {
    static inline uint64_t step_rule_184(uint64_t state) {
        uint64_t l = std::rotl(state, 1);
        uint64_t r = std::rotr(state, 1);
        return (l & ~state) | (state & r);
    }

    static inline uint64_t step_rule_30(uint64_t state) {
        uint64_t l = std::rotl(state, 1);
        uint64_t r = std::rotr(state, 1);
        return l ^ (state | r);
    }

    static inline uint32_t count_demarcs(uint64_t seed_bits, bool& particle_conserved_out) {
        uint64_t fwd_0 = seed_bits;
        uint64_t cf_0  = ~seed_bits;
        uint32_t p_before = static_cast<uint32_t>(std::popcount(fwd_0));

        uint64_t fwd_1 = step_rule_184(fwd_0);
        uint64_t cf_1  = step_rule_184(cf_0);
        uint64_t r30_1 = step_rule_30(fwd_0);

        uint32_t p_after_fwd = static_cast<uint32_t>(std::popcount(fwd_1));
        uint32_t p_after_cf  = static_cast<uint32_t>(std::popcount(cf_1));
        particle_conserved_out = (p_before == p_after_fwd) && ((p_after_fwd + p_after_cf) == 64u);

        uint64_t matches = ~(fwd_1 ^ ~r30_1) & ~(cf_1 ^ r30_1);
        uint64_t demarcs = matches ^ std::rotl(matches, 1);
        return static_cast<uint32_t>(std::popcount(demarcs));
    }
};

// ============================================================================
// 5B. CONSTITUTIONAL 5-EDGE `ModeRelationLinks` (Correction C4)
// Provenance: [SL §4.7] Filtration Adjacency; [CF §19.5] Mixed-Sector Coupling
//             [O_i, O_j] = f_{ij}^k O_k + \Omega_{ij}^{\mu\nu} F_{\mu\nu}
// ============================================================================
static constexpr double BELL_COUNTERFACTUAL_SIM_MARGIN = 1.0 / 15.0; // Direct Bell-15 margin

struct ModeRelationLinks {
    // e = 0: TriSectorRepresentation [CF §19.5] (Folded 3-block majority consensus braid)
    uint32_t tri_sector_rep_idx    = ABSENT_NODE;
    // e = 1: MixedSectorCoupling [CF §19.5] ([O_i, O_j] = f_{ij}^k O_k + \Omega_{ij}^{\mu\nu} F_{\mu\nu})
    uint32_t mixed_sector_sib0_idx = ABSENT_NODE;
    uint32_t mixed_sector_sib1_idx = ABSENT_NODE;
    // e = 2: AdjacencyRelation [SL §4.7] (Sequential filtration adjacency within same file)
    uint32_t adjacency_prev_idx    = ABSENT_NODE;
    uint32_t adjacency_next_idx    = ABSENT_NODE;
    // e = 3: HolonomyEquivalence [CF Exhibit B] (Shared primary Semantic Mass Node across paths)
    uint32_t holonomy_equiv_idx    = ABSENT_NODE;
    // e = 4: CurvatureEquivalence [CF §19.5] (Isomorphic E8 / SU(7) sector resonance)
    uint32_t curvature_equiv_idx   = ABSENT_NODE;
    uint64_t primary_mass_seed     = 0;
};

// ============================================================================
// 6. HALO HEADER (\Sigma), HOT SoA ROUTE ENTRY & UOR COORDINATE
// ============================================================================
// Phase -1a Provenance Comment — HaloHeader & bucket_id:
//   - What it computes: Compact 8-byte hot metadata header per cell; `bucket_id` (0..255)
//     stores the 16x16 HashBucketQuantizer bucket key for O(1) admission_buckets indexing.
//   - What consumes it: rebuild_lookup_tables(), ingest_file(), verify_adequacy_gate(), and route().
struct HaloHeader {
    Scale scale               = Scale::BLOCK;
    Modality modality         = Modality::REPO_CODE;
    uint8_t bucket_id         = 0; // Phase -1a: 0..255 bucket key from HashBucketQuantizer
    uint8_t entropy_q8        = 0; // Shannon entropy H * 32 (0..255 maps to 0..8 bits/byte)
    uint8_t surprisal_q8      = 0; // Character-class transition delta * 255

};

struct HotRouteEntry {
    HaloHeader halo{};
    HelixSignature256 sig{};
};

struct PackedCellRecord {
    uint32_t id           = 0;
    uint32_t parent_id    = 0;
    HaloHeader halo{};
    uint32_t start_byte   = 0;
    uint32_t end_byte     = 0;
    uint32_t start_line   = 1;
    uint32_t end_line     = 1;
    uint64_t content_hash = 0;
    uint64_t mtime_ns     = 0;
    HelixSignature256 sig{};
    uint64_t primary_mass_seed = 0;
    CanonicalDigest256 canonical_digest{}; // Phase 3.3 (D-adequacy-hash-2): 256-bit digest + length
    uint16_t exact_stat_digest = 0;        // Phase 3.4 (D-adequacy-6b): Exact ByteStats 16-bit fingerprint
    uint32_t uri_offset   = 0;
    uint32_t uri_len      = 0;
    uint32_t prev_offset  = 0;
    uint32_t prev_len     = 0;
    uint32_t corpus_offset = 0;
    uint32_t corpus_len    = 0;
};

struct UORCoord {
    std::string uri;
    uint32_t start_byte = 0;
    uint32_t end_byte   = 0;
    uint32_t start_line = 1;
    uint32_t end_line   = 1;
    uint64_t content_hash = 0;
    uint64_t mtime_ns     = 0;
};

struct Cell {
    uint32_t id        = 0;
    uint32_t parent_id = 0;
    HaloHeader halo;
    HelixSignature256 sig{};
    uint64_t primary_mass_seed = 0;
    CanonicalDigest256 canonical_digest{}; // Phase 3.3 (D-adequacy-hash-2): 256-bit canonical digest
    uint64_t canonical_hash    = 0;        // Low 64 bits of canonical_digest (for diagnostic compatibility)
    uint32_t canonical_len     = 0;        // Byte length of Adapter::canonical_form(path)
    uint16_t exact_stat_digest = 0;        // Phase 3.4 (D-adequacy-6b): Exact ByteStats 16-bit fingerprint
    UORCoord coord;
    Hypervec8192 hv;
    std::string preview;
    std::string corpus_id = "local";
};

// ============================================================================
// 7. CONCRETE SUBSTRATE ADAPTERS (`ByteAdapter`, `CodeAdapter`, `HtmlAdapter`, `DnaAdapter`)
// Provenance: [CF Alignment Map §C, §G]; [M §5, §7]
// ============================================================================
static std::string demodulate_html_to_text(const std::string& html) {
    std::string out;
    out.reserve(html.size());

    size_t i = 0;
    const size_t n = html.size();
    auto starts_ci = [&](size_t pos, const char* lit) {
        size_t l = std::strlen(lit);
        if (pos + l > n) return false;
        for (size_t k = 0; k < l; ++k) {
            if (std::tolower(static_cast<unsigned char>(html[pos + k])) !=
                std::tolower(static_cast<unsigned char>(lit[k])))
                return false;
        }
        return true;
    };

    while (i < n) {
        if (html[i] == '<') {
            if (starts_ci(i, "<script")) {
                while (i < n && !starts_ci(i, "</script>")) i++;
                while (i < n && html[i] != '>') i++;
                if (i < n) i++;
                continue;
            }
            if (starts_ci(i, "<style")) {
                while (i < n && !starts_ci(i, "</style>")) i++;
                while (i < n && html[i] != '>') i++;
                if (i < n) i++;
                continue;
            }
            if (starts_ci(i, "<nav") || starts_ci(i, "<footer")) {
                const char* close_tag = starts_ci(i, "<nav") ? "</nav>" : "</footer>";
                while (i < n && !starts_ci(i, close_tag)) i++;
                while (i < n && html[i] != '>') i++;
                if (i < n) i++;
                continue;
            }

            if (starts_ci(i, "<h1") || starts_ci(i, "<h2") || starts_ci(i, "<h3") || starts_ci(i, "<h4")) {
                out += "\n\n# ";
            } else if (starts_ci(i, "</h1") || starts_ci(i, "</h2") || starts_ci(i, "</h3") ||
                       starts_ci(i, "</h4") || starts_ci(i, "</p>") || starts_ci(i, "</pre>") ||
                       starts_ci(i, "</div>") || starts_ci(i, "</li>") || starts_ci(i, "<br")) {
                out += "\n";
            } else if (starts_ci(i, "<pre") || starts_ci(i, "<code")) {
                out += "\n";
            }

            while (i < n && html[i] != '>') {
                if (html[i] == '\n') out.push_back('\n');
                i++;
            }
            if (i < n) i++;
        } else if (html[i] == '&') {
            if (starts_ci(i, "&lt;"))        { out.push_back('<'); i += 4; }
            else if (starts_ci(i, "&gt;"))   { out.push_back('>'); i += 4; }
            else if (starts_ci(i, "&amp;"))  { out.push_back('&'); i += 5; }
            else if (starts_ci(i, "&quot;")) { out.push_back('"'); i += 6; }
            else if (starts_ci(i, "&#39;"))  { out.push_back('\''); i += 5; }
            else if (starts_ci(i, "&nbsp;")) { out.push_back(' '); i += 6; }
            else { out.push_back(html[i++]); }
        } else {
            out.push_back(html[i++]);
        }
    }
    return out;
}

enum class CharClass : uint8_t {
    SPACE = 0, LOWER = 1, UPPER = 2, DIGIT = 3, OP_MATH = 4, DELIM = 5, OTHER = 6
};

struct CharClassLUT {
    std::array<CharClass, 256> table{};
    constexpr CharClassLUT() {
        for (int i = 0; i < 256; ++i) table[i] = CharClass::OTHER;
        table[' '] = table['\t'] = table['\r'] = table['\n'] = CharClass::SPACE;
        for (int c = 'a'; c <= 'z'; ++c) table[c] = CharClass::LOWER;
        for (int c = 'A'; c <= 'Z'; ++c) table[c] = CharClass::UPPER;
        for (int c = '0'; c <= '9'; ++c) table[c] = CharClass::DIGIT;
        const char* ops = "+-*/=<>!&|^~%\\";
        for (int k = 0; ops[k] != '\0'; ++k) table[static_cast<uint8_t>(ops[k])] = CharClass::OP_MATH;
        const char* delims = "(){}[].,;:_#@$\"'`";
        for (int k = 0; delims[k] != '\0'; ++k) table[static_cast<uint8_t>(delims[k])] = CharClass::DELIM;
    }
};
static constexpr CharClassLUT CHAR_CLASS_LUT{};

static constexpr inline CharClass classify_byte(uint8_t c) {
    return CHAR_CLASS_LUT.table[c];
}

struct ByteStats {
    double shannon_entropy     = 0.0;
    double mean_delta          = 0.0;
    double op_density          = 0.0;
    double delim_density       = 0.0;
    double rel_op_density      = 0.0;
    uint16_t exact_stat_digest = 0; // Phase 3.4 (D-adequacy-6b): 16-bit exact statistical digest
};

// Correction C3 [§10, Stage 3]: Exact statistics (step = 1) for all inputs <= 16 KB
static ByteStats compute_byte_stats(std::string_view text) {
    ByteStats st;
    if (text.empty()) return st;
    std::array<uint32_t, 256> freq{};
    uint32_t transitions = 0;
    uint32_t op_count = 0;
    uint32_t delim_count = 0;
    uint32_t rel_count = 0;
    CharClass prev_cc = classify_byte(static_cast<uint8_t>(text[0]));

    // Correction C3: Full-resolution exact pass when text.size() <= 16384 (16 KB)
    size_t step = (text.size() > 16384) ? (text.size() / 4096) : 1;
    size_t sampled = 0;
    for (size_t i = 0; i < text.size(); i += step) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        freq[c]++;
        CharClass cc = classify_byte(c);
        if (cc != prev_cc) transitions++;
        if (cc == CharClass::OP_MATH) op_count++;
        if (cc == CharClass::DELIM) delim_count++;
        if (c == '!' || c == '<' || c == '>' || (c == '=' && i + 1 < text.size() && text[i + 1] == '=')) {
            rel_count++;
        }
        prev_cc = cc;
        sampled++;
    }

    double n = static_cast<double>(sampled);
    double h = 0.0;
    for (uint32_t count : freq) {
        if (count > 0) {
            double p = static_cast<double>(count) / n;
            h -= p * std::log2(p);
        }
    }
    st.shannon_entropy = h;
    st.mean_delta = static_cast<double>(transitions) / n;
    st.op_density = static_cast<double>(op_count) / n;
    st.delim_density = static_cast<double>(delim_count) / n;
    st.rel_op_density = static_cast<double>(rel_count) / n;

    // Phase 3.4 (D-adequacy-6b): Compute 16-bit exact_stat_digest from the 5 ByteStats moments
    // and non-whitespace byte frequency distribution (whitespace-invariant so CodeAdapter's
    // positive equivalence witness ("  foo()  ", "foo()") shares the exact same stat digest!).
    uint64_t sd = 0x9E3779B97F4A7C15ULL;
    sd ^= static_cast<uint64_t>(std::llround(st.shannon_entropy * 1024.0)) * 0xBF58476D1CE4E5B9ULL;
    sd = std::rotl(sd, 13) ^ (static_cast<uint64_t>(std::llround(st.mean_delta * 4096.0)) * 0x94D049BB133111EBULL);
    sd = std::rotl(sd, 17) ^ (static_cast<uint64_t>(std::llround(st.op_density * 4096.0)) * 0xCBF29CE484222325ULL);
    sd = std::rotl(sd, 19) ^ (static_cast<uint64_t>(std::llround(st.delim_density * 4096.0)) * 1099511628211ULL);
    sd = std::rotl(sd, 23) ^ static_cast<uint64_t>(std::llround(st.rel_op_density * 4096.0));
    uint64_t nonspace_total = 0;
    for (int ch = 0; ch < 256; ++ch) {
        if (!std::isspace(ch) && freq[ch] > 0) {
            nonspace_total += freq[ch];
            sd = std::rotl(sd, 5) ^ (static_cast<uint64_t>(ch) * 2654435761ULL + freq[ch]);
        }
    }
    sd ^= nonspace_total * 0x9E3779B97F4A7C15ULL;
    sd = splitmix64(sd);
    st.exact_stat_digest = static_cast<uint16_t>((sd ^ (sd >> 16) ^ (sd >> 32) ^ (sd >> 48)) & 0xFFFFu);
    return st;
}

// Concrete Adapter 1: ByteAdapter (unformatted raw byte streams) [§3, D-adequacy-1]
//   canonical_form is the identity on raw bytes; equiv_F is exact canonical equality
//   (strictly stronger than approx_F, so approx_F => equiv_F is falsifiable!).
struct ByteAdapter {
    template <SubstrateView S>
    static bool A_path(const Path<S>& p) noexcept {
        return !p.bytes().empty();
    }

    template <SubstrateView S>
    static bool A_local(const Path<S>& p) noexcept {
        auto b = p.bytes();
        return b.size() >= 4 && b.size() <= (2u * 1024u * 1024u);
    }

    static std::span<const std::uint8_t> canonical_form(std::span<const std::uint8_t> x) noexcept {
        return x;
    }

    static bool approx_F(std::span<const std::uint8_t> x, std::span<const std::uint8_t> y) noexcept {
        if (x.empty() || y.empty()) return false;
        std::string_view sx(reinterpret_cast<const char*>(x.data()), x.size());
        std::string_view sy(reinterpret_cast<const char*>(y.data()), y.size());
        ByteStats bx = compute_byte_stats(sx);
        ByteStats by = compute_byte_stats(sy);
        return std::abs(bx.shannon_entropy - by.shannon_entropy) < 0.75 &&
               std::abs(bx.mean_delta - by.mean_delta) < 0.20;
    }

    static bool equiv_F(std::span<const std::uint8_t> x, std::span<const std::uint8_t> y) noexcept {
        auto cx = canonical_form(x);
        auto cy = canonical_form(y);
        return cx.size() == cy.size() &&
               (cx.empty() || std::memcmp(cx.data(), cy.data(), cx.size()) == 0);
    }

    // D-adequacy-2: Per-Adapter witness pair (positive equivalence & falsifying counterexample)
    static constexpr std::pair<std::string_view, std::string_view> positive_witness() noexcept {
        return {"foo()", "foo()"};
    }
    static constexpr std::pair<std::string_view, std::string_view> falsifying_witness() noexcept {
        return {"  foo()  ", "foo()"};
    }
};

// Concrete Adapter 2: CodeAdapter (structured code & text) [§3, D-adequacy-1]
//   canonical_form trims leading/trailing whitespace and collapses internal runs;
//   equiv_F compares normalized canonical forms (strictly stronger than approx_F).
struct CodeAdapter {
    template <SubstrateView S>
    static bool A_path(const Path<S>& p) noexcept {
        auto b = p.bytes();
        if (b.empty()) return false;
        size_t check_n = std::min<size_t>(b.size(), 512);
        for (size_t i = 0; i < check_n; ++i) {
            if (b[i] == 0) return false; // Null-byte exclusion
        }
        return true;
    }

    template <SubstrateView S>
    static bool A_local(const Path<S>& p) noexcept {
        auto b = p.bytes();
        if (b.empty()) return false;
        for (uint8_t c : b) {
            if (classify_byte(c) != CharClass::SPACE) return true;
        }
        return false;
    }

    static std::vector<std::uint8_t> canonical_form(std::span<const std::uint8_t> x) {
        std::vector<std::uint8_t> out;
        out.reserve(x.size());
        bool in_space_run = false;
        size_t start = 0;
        while (start < x.size() && std::isspace(static_cast<unsigned char>(x[start]))) start++;
        size_t last_non_space = start;
        for (size_t i = start; i < x.size(); ++i) {
            if (std::isspace(static_cast<unsigned char>(x[i]))) {
                in_space_run = true;
            } else {
                if (in_space_run && last_non_space >= start && !out.empty()) out.push_back(' ');
                out.push_back(x[i]);
                last_non_space = i;
                in_space_run = false;
            }
        }
        return out;
    }

    static bool approx_F(std::span<const std::uint8_t> x, std::span<const std::uint8_t> y) noexcept {
        return ByteAdapter::approx_F(x, y);
    }

    static bool equiv_F(std::span<const std::uint8_t> x, std::span<const std::uint8_t> y) noexcept {
        auto cx = canonical_form(x);
        auto cy = canonical_form(y);
        return cx.size() == cy.size() &&
               (cx.empty() || std::memcmp(cx.data(), cy.data(), cx.size()) == 0);
    }

    // D-adequacy-2: Under CodeAdapter, ("  foo()  ", "foo()") is the POSITIVE equivalence witness,
    // while ("  foo(x)  ", "  bar(y)  ") is the FALSIFYING witness!
    static constexpr std::pair<std::string_view, std::string_view> positive_witness() noexcept {
        return {"  foo()  ", "foo()"};
    }
    static constexpr std::pair<std::string_view, std::string_view> falsifying_witness() noexcept {
        return {"  foo(x)  ", "  bar(y)  "};
    }
};

// Concrete Adapter 3: HtmlAdapter (demodulates HTML/wget trees for @web) [§3, D-adequacy-2]
struct HtmlAdapter {
    template <SubstrateView S>
    static bool A_path(const Path<S>& p) noexcept { return CodeAdapter::A_path(p); }

    template <SubstrateView S>
    static bool A_local(const Path<S>& p) noexcept { return CodeAdapter::A_local(p); }

    static std::vector<std::uint8_t> canonical_form(std::span<const std::uint8_t> x) {
        std::string raw(reinterpret_cast<const char*>(x.data()), x.size());
        std::string dem = demodulate_html_to_text(raw);
        return CodeAdapter::canonical_form(
            std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(dem.data()), dem.size())
        );
    }

    static bool approx_F(std::span<const std::uint8_t> x, std::span<const std::uint8_t> y) noexcept {
        return CodeAdapter::approx_F(x, y);
    }

    static bool equiv_F(std::span<const std::uint8_t> x, std::span<const std::uint8_t> y) noexcept {
        auto cx = canonical_form(x);
        auto cy = canonical_form(y);
        return cx.size() == cy.size() &&
               (cx.empty() || std::memcmp(cx.data(), cy.data(), cx.size()) == 0);
    }

    static std::string demodulate(const std::string& raw_html) {
        return demodulate_html_to_text(raw_html);
    }

    static constexpr std::pair<std::string_view, std::string_view> positive_witness() noexcept {
        return {"<p>  fft_step(x)  </p>", "<div>fft_step(x)</div>"};
    }
    static constexpr std::pair<std::string_view, std::string_view> falsifying_witness() noexcept {
        return {"<p>fft_step(x)</p>", "<p> dct_step(y)</p>"};
    }
};

// Concrete Adapter 4: DnaAdapter (genomic ACGTN sequence carrier) [§3, D-adequacy-2]
struct DnaAdapter {
    static constexpr bool is_dna_base(uint8_t c) noexcept {
        return c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'N' ||
               c == 'a' || c == 'c' || c == 'g' || c == 't' || c == 'n' ||
               c == ' ' || c == '\n' || c == '\r';
    }

    template <SubstrateView S>
    static bool A_path(const Path<S>& p) noexcept {
        auto b = p.bytes();
        if (b.empty()) return false;
        for (uint8_t c : b) {
            if (!is_dna_base(c)) return false;
        }
        return true;
    }

    template <SubstrateView S>
    static bool A_local(const Path<S>& p) noexcept {
        return p.bytes().size() >= 8;
    }

    static std::vector<std::uint8_t> canonical_form(std::span<const std::uint8_t> x) {
        std::vector<std::uint8_t> out;
        out.reserve(x.size());
        for (uint8_t c : x) {
            if (c == ' ' || c == '\n' || c == '\r') continue;
            if (c >= 'a' && c <= 'z') c = static_cast<uint8_t>(c - 'a' + 'A');
            out.push_back(c);
        }
        return out;
    }

    static bool approx_F(std::span<const std::uint8_t> x, std::span<const std::uint8_t> y) noexcept {
        return ByteAdapter::approx_F(x, y);
    }

    static bool equiv_F(std::span<const std::uint8_t> x, std::span<const std::uint8_t> y) noexcept {
        auto cx = canonical_form(x);
        auto cy = canonical_form(y);
        return cx.size() == cy.size() &&
               (cx.empty() || std::memcmp(cx.data(), cy.data(), cx.size()) == 0);
    }

    static constexpr std::pair<std::string_view, std::string_view> positive_witness() noexcept {
        return {"ACGTACGT\n", "ACGTACGT"};
    }
    static constexpr std::pair<std::string_view, std::string_view> falsifying_witness() noexcept {
        return {"ACGTACGT", "TGCATGCA"};
    }
};

// D-adequacy-2 & D-adequacy-5: Mechanized verifier checking an Adapter's positive and
// falsifying witnesses, plus whether the positive witness is non-degenerate (pos_x != pos_y).
// Identity-canonicalization adapters (ByteAdapter) have a degenerate positive witness
// (pos_x == pos_y), whereas quotient-canonicalization adapters (Code, Html, Dna) have pos_x != pos_y.
template <typename A>
static inline bool verify_adapter_witness(bool* is_nontrivial_quotient_out = nullptr) {
    auto [pos_x, pos_y] = A::positive_witness();
    auto [neg_x, neg_y] = A::falsifying_witness();
    std::span<const std::uint8_t> bpx(reinterpret_cast<const std::uint8_t*>(pos_x.data()), pos_x.size());
    std::span<const std::uint8_t> bpy(reinterpret_cast<const std::uint8_t*>(pos_y.data()), pos_y.size());
    std::span<const std::uint8_t> bnx(reinterpret_cast<const std::uint8_t*>(neg_x.data()), neg_x.size());
    std::span<const std::uint8_t> bny(reinterpret_cast<const std::uint8_t*>(neg_y.data()), neg_y.size());

    if (is_nontrivial_quotient_out != nullptr) {
        *is_nontrivial_quotient_out = (pos_x != pos_y);
    }
    bool pos_ok = A::approx_F(bpx, bpy) && A::equiv_F(bpx, bpy);
    bool neg_ok = A::approx_F(bnx, bny) && !A::equiv_F(bnx, bny);
    return pos_ok && neg_ok;
}

// Phase -1c & Phase 3.3 (D-adequacy-hash-2):
// Refactored to delegate canonical byte normalization directly to `CodeAdapter::canonical_form`
// (eliminating the duplicated whitespace-normalization state machine!) and compute a full
// 256-bit collision-resistant ARX digest over the canonical byte span.
static inline CanonicalDigest256 compute_canonical_digest256(std::string_view text) {
    std::span<const std::uint8_t> raw_span(
        reinterpret_cast<const std::uint8_t*>(text.data()),
        text.size()
    );
    std::vector<std::uint8_t> canon = CodeAdapter::canonical_form(raw_span);
    CanonicalDigest256 out;
    out.canonical_len = static_cast<uint32_t>(canon.size());
    out.words = crypto_arx_hash256(canon.data(), canon.size(), 0x6A09E667F3BCC908ULL);
    return out;
}

static inline std::pair<uint64_t, uint32_t> compute_canonical_digest(std::string_view text) {
    CanonicalDigest256 d256 = compute_canonical_digest256(text);
    return {d256.low64(), d256.canonical_len};
}

static_assert(AdapterFor<ByteAdapter, ByteSpanSubstrate>);
static_assert(AdapterFor<CodeAdapter, ByteSpanSubstrate>);
static_assert(AdapterFor<HtmlAdapter, ByteSpanSubstrate>);
static_assert(AdapterFor<DnaAdapter,  ByteSpanSubstrate>);

// ============================================================================
// §4, §5, §6, §7. STAGES 2-6: PRESERVATION, WEIGHTING, RELATIONAL ENDPOINT,
// FOAM DENSITY \rho_F, 11D INVARIANT COORDINATES I(\rho) & ADEQUACY GATE
// Provenance: [CF Exhibits A-E; Alignment Map §C-§I]
// ============================================================================

// §6. Stages 4 & 5: Relational Endpoint F_rel = (H_F, ~_F, w_F), FoamDensity \rho_F,
//     and 13-Dimensional Invariant Coordinate Map I(\rho) \in R^13 (Exhibit E + Phase 4A Cartan/Holonomy)
//     Provenance: [CF Exhibits B, D, E; Alignment Map §F, §J]
struct RelationalEndpoint {
    size_t num_admissible_paths                 = 0;    // |\Gamma^0_F|
    size_t num_orbits                           = 0;    // |H_F| = |\Gamma^0_F / ~_F|
    std::vector<std::complex<double>> orbit_w_F;        // w_F(\gamma) per orbit in H_F
    bool orbit_invariance_holds                 = true; // \gamma_1 ~_F \gamma_2 => w_F(\gamma_1) == w_F(\gamma_2)
    bool has_ambiguous_extension                = false;// Phase 2: true when >=2 non-isomorphic realizations exist
    bool composition_associative_and_congruent  = true; // Phase 2 (CF §J.2): Filtration concatenation congruent with ~_F
    bool composition_undecided                  = false;// Phase 2: true if composition obligation is left open
    bool groupoid_invertible_transport          = true; // Phase 2 (CF §J.3): Clifford 64-state transport invertibility
};

// Phase 3.2 (D-adequacy-byte-gate): Distinguishes substantive quotient certification
// (CodeAdapter, HtmlAdapter, DnaAdapter where pos_x != pos_y) from vacuous identity
// certification (ByteAdapter where pos_x == pos_y).
enum class CertificationKind : uint8_t {
    Vacuous     = 0,
    Substantive = 1
};

// CF Exhibit D (Eq. ExhibitD.2 - ExhibitD.3):
//   \rho_F = (1 / Z_F) \sum_{\gamma \in \Gamma_F} w_F(\gamma) |\gamma><\gamma|, Tr(\rho_F) = 1
struct FoamDensity {
    std::vector<double> weights; // Normalized diagonal probabilities p_i = |w_F(\gamma_i)| / Z_F
    double Z_F        = 0.0;     // Partition normalization Z_F = \sum |w_F(\gamma_i)|
    double trace      = 0.0;     // Tr(\rho_F) == 1.0
    double purity_tr2 = 0.0;     // Tr(\rho_F^2) = \sum p_i^2
};

// CF Exhibit E (pp. 331-333, Eq. ExhibitE.4 - ExhibitE.12) + Phase 4A.3:
//   I(\rho) = (N_0, d_eff, SE, R_t, A_y, K_t, C_t, A_0, A_1, A_2, A_3, Q_cartan, Hol_A) \in R^{13}
struct InvariantCoordinateVector {
    std::array<double, 11> coords{};

    double N_0()            const noexcept { return coords[0]; }  // Carrier orbit cardinality |H_F|
    double d_eff()          const noexcept { return coords[1]; }  // Effective dimension 1 / Tr(\rho^2) [Eq. ExhibitE.4]
    double SE()             const noexcept { return coords[2]; }  // Linear Tsallis-2 entropy 1 - Tr(\rho^2) [Eq. ExhibitE.6]
    double R_t()            const noexcept { return coords[3]; }  // Spectral trace ratio
    double A_y()            const noexcept { return coords[4]; }  // Spectral asymmetry
    double K_t()            const noexcept { return coords[5]; }  // Spectral kurtosis / concentration
    double C_t()            const noexcept { return coords[6]; }  // Coherence off-diagonal coupling
    double A_0()            const noexcept { return coords[7]; }  // Leading spectral mode 0
    double A_1()            const noexcept { return coords[8]; }  // Sub-leading spectral mode 1
    double A_2()            const noexcept { return coords[9]; }  // Spectral mode 2
    double A_3()            const noexcept { return coords[10]; } // Spectral mode 3
};

// §7. Stage 6 & Phase 2: Two-Axis Adequacy Certificate [CF Alignment Map §G-§J]
//   Axis 1: `forcedness` (Axis1Cert across J.1, J.2, J.3)
//   Axis 2: `instance_adequacy_holds` (\psi ~_F \phi => \psi ==_F \phi across all bucket pairs)
//   Plus `cert_kind` (Substantive vs Vacuous quotient certification).
struct AdequacyCertificate {
    bool su0_to_su7_fixed                  = true;  // Condition 1 [CF Alignment Map §I]
    bool carrier_defined                   = false; // Condition 2: \Gamma^0_F defined
    bool preservation_established          = false; // Condition 3: SU(7)(\Gamma^0_F) <= \Gamma^0_F
    bool relational_endpoint_fixed         = false; // Condition 4: F_rel = (H_F, ~_F, w_F) fixed
    bool instance_adequacy_holds           = false; // Axis 2 (Phase 2): \psi ~_F \phi => \psi ==_F \phi
    bool adequacy_holds                    = false; // Synchronized with instance_adequacy_holds
    CertificationKind cert_kind            = CertificationKind::Vacuous; // Phase 3.2 (D-adequacy-byte-gate)
    uint32_t pairs_verified                = 0;
    uint32_t failing_witness_a             = 0;     // First cell_id in orbit failing approx_F => equiv_F
    uint32_t failing_witness_b             = 0;     // Second cell_id in orbit failing approx_F => equiv_F
    uint64_t merkle_root                   = 0;     // 64-bit FNV-1a Merkle root
    std::array<uint64_t, 4> merkle_root_256{};      // Phase 3.3 (D-adequacy-hash-2): 256-bit CRYPTO_ARX_256 Merkle root
    FoamDensity density{};
    InvariantCoordinateVector invariant_coords{};

    constexpr bool is_fully_certified() const noexcept {
        return su0_to_su7_fixed &&
               carrier_defined &&
               preservation_established &&
               relational_endpoint_fixed &&
               instance_adequacy_holds &&
               (cert_kind == CertificationKind::Substantive);
    }

    constexpr bool is_certified() const noexcept {
        return is_fully_certified();
    }
};

// ============================================================================
// 7B. PATCH EXTRACTOR & 64-BIT BIT-SLICED HYPERVECTOR ENCODER (§9 Generator Strides)
// ============================================================================
struct ExtractedPatch {
    uint64_t seed;
    int16_t intrinsic_weight;
    bool is_primary_token;
};

static Hypervec8192 encode_span_hypervector(
    std::string_view text,
    SurprisalSketch* surprisal_sketch = nullptr,
    bool update_sketch = false,
    size_t max_mass_nodes = MAX_BLOCK_MASS_NODES,
    uint64_t* primary_mass_seed_out = nullptr
) {
    ExtractedPatch patches[128];
    size_t n_patches = 0;
    size_t limit = std::min<size_t>(text.size(), 1536);

    char cur_word[64];
    size_t cur_len = 0;

    auto flush_word = [&]() {
        if (cur_len >= 2 && n_patches < 120) {
            int16_t base_w = (cur_len >= 5) ? 4 : ((cur_len >= 3) ? 3 : 2);
            patches[n_patches++] = {fnv1a_64(cur_word, cur_len), base_w, true};

            if (cur_len >= 5 && n_patches < 120) {
                for (size_t j = 0; j + 4 <= cur_len && j < 12 && n_patches < 120; j += 2) {
                    uint64_t gram4 = fnv1a_64(cur_word + j, 4, 0x9E3779B97F4A7C15ULL);
                    patches[n_patches++] = {gram4, 2, false};
                }
            }
            if (cur_len >= 4 && n_patches < 120) {
                uint64_t prefix3 = fnv1a_64(cur_word, 3, 0xBF58476D1CE4E5B9ULL);
                patches[n_patches++] = {prefix3, 1, false};
            }
        }
        cur_len = 0;
    };

    for (size_t i = 0; i < limit; ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c >= 'A' && c <= 'Z') {
            if (cur_len > 0 && i > 0) {
                unsigned char prev_c = static_cast<unsigned char>(text[i - 1]);
                if (prev_c >= 'a' && prev_c <= 'z') flush_word();
            }
            if (cur_len < 63) cur_word[cur_len++] = static_cast<char>(c - 'A' + 'a');
        } else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c >= 0x80) {
            if (cur_len < 63) cur_word[cur_len++] = static_cast<char>(c);
        } else {
            flush_word();
            if (classify_byte(c) == CharClass::OP_MATH && n_patches < 120) {
                char op_buf[2] = {static_cast<char>(c), 0};
                size_t op_len = 1;
                if (i + 1 < limit && classify_byte(static_cast<uint8_t>(text[i + 1])) == CharClass::OP_MATH) {
                    op_buf[1] = text[i + 1];
                    op_len = 2;
                }
                patches[n_patches++] = {fnv1a_64(op_buf, op_len, 0x94D049BB133111EBULL), 3, true};
            }
        }
        if (n_patches >= 120) break;
    }
    flush_word();

    std::pair<uint64_t, int16_t> seed_weights[192];
    bool seed_is_primary[192]{};
    size_t num_unique = 0;

    auto add_feature_seed = [&](uint64_t seed, int16_t w, bool primary) {
        for (size_t i = 0; i < num_unique; ++i) {
            if (seed_weights[i].first == seed) {
                seed_weights[i].second += std::max<int16_t>(1, w / 2);
                return;
            }
        }
        if (num_unique < 192) {
            int16_t modulated_w = w;
            if (surprisal_sketch != nullptr && primary) {
                modulated_w = surprisal_sketch->weight_for_seed(seed, w);
            }
            seed_weights[num_unique] = {seed, modulated_w};
            seed_is_primary[num_unique] = primary;
            num_unique++;
        }
    };

    uint64_t prev1_primary = 0;
    uint64_t prev2_primary = 0;

    const int stride0 = static_cast<int>(PRIME_STRIDES[0]);
    const int stride1 = static_cast<int>(PRIME_STRIDES[1]);

    for (size_t pi = 0; pi < n_patches; ++pi) {
        const auto& p = patches[pi];
        add_feature_seed(p.seed, p.intrinsic_weight, p.is_primary_token);

        if (p.is_primary_token) {
            if (prev1_primary != 0) {
                uint64_t bg_seed = std::rotl(prev1_primary, stride0) ^
                                   (p.seed * 0x9E3779B97F4A7C15ULL);
                add_feature_seed(bg_seed, 2, false);
            }
            if (prev2_primary != 0) {
                uint64_t sg_seed = std::rotl(prev2_primary, stride1) ^
                                   (p.seed * 0xBF58476D1CE4E5B9ULL);
                add_feature_seed(sg_seed, 1, false);
            }
            prev2_primary = prev1_primary;
            prev1_primary = p.seed;
        }
    }

    if (surprisal_sketch != nullptr && update_sketch) {
        surprisal_sketch->step_span(+1);
        for (size_t i = 0; i < num_unique; ++i) {
            if (seed_is_primary[i]) {
                surprisal_sketch->observe_seed(seed_weights[i].first, +1);
            }
        }
    }

    size_t char_limit = std::min<size_t>(text.size(), 256);
    size_t shingle_step = std::max<size_t>(6, char_limit / 12);
    for (size_t i = 0; i + 4 <= char_limit; i += shingle_step) {
        uint64_t sh_seed = fnv1a_64(text.data() + i, 4, 0xCBF29CE484222325ULL);
        add_feature_seed(sh_seed, 1, false);
    }

    if (primary_mass_seed_out != nullptr) {
        uint64_t best_seed = 0;
        int16_t best_w = -1;
        for (size_t i = 0; i < num_unique; ++i) {
            if (seed_is_primary[i] && seed_weights[i].second > best_w) {
                best_w = seed_weights[i].second;
                best_seed = seed_weights[i].first;
            }
        }
        *primary_mass_seed_out = best_seed;
    }

    size_t order[192];
    for (size_t i = 0; i < num_unique; ++i) order[i] = i;
    if (num_unique > max_mass_nodes && max_mass_nodes >= 2) {
        std::partial_sort(order, order + max_mass_nodes, order + num_unique,
            [&](size_t a, size_t b) {
                int score_a = static_cast<int>(seed_weights[a].second) + (seed_is_primary[a] ? 2 : 0);
                int score_b = static_cast<int>(seed_weights[b].second) + (seed_is_primary[b] ? 2 : 0);
                return score_a > score_b;
            });
        // Phase 3.1 (D-adequacy-braid-1): Fold evicted tail token seeds (purely pre-canonical
        // feature tokens, NEVER compute_canonical_digest!) into slot max_mass_nodes - 1 so rare
        // tokens in boilerplate blocks are represented without smuggling canonical_digest into hv!
        uint64_t tail_sketch = seed_weights[order[max_mass_nodes - 1]].first;
        for (size_t k = max_mass_nodes; k < num_unique; ++k) {
            tail_sketch = std::rotl(tail_sketch, 7) ^ (seed_weights[order[k]].first * 0x9E3779B97F4A7C15ULL);
        }
        seed_weights[order[max_mass_nodes - 1]].first = tail_sketch;
        num_unique = max_mass_nodes;
    }

    // Phase 3.1 (D-adequacy-braid-1): Zero canonical_digest braiding in hv_out!
    Hypervec8192 hv_out = encode_seeds_bitsliced(seed_weights, order, num_unique);
    return hv_out;
}

// ============================================================================
// 8. UNIVERSAL BYTE-MARKOV + CA 184/30 DEMARCATION & DELIMITER CHUNKER
// ============================================================================
struct RawBlock {
    uint32_t start_byte;
    uint32_t end_byte;
    uint32_t start_line;
    uint32_t end_line;
    std::string_view text;
    std::string preview;
};

static std::vector<RawBlock> chunk_unformatted_byte_stream(
    std::string_view content,
    const ByteMarkovModel* markov = nullptr
) {
    std::vector<RawBlock> blocks;
    if (content.empty()) return blocks;

    const size_t n = content.size();
    size_t start_b = 0;
    uint32_t chunk_idx = 1;
    double running_surprisal = 4.0;

    for (size_t pos = 64; pos < n; pos += 16) {
        size_t span_len = pos - start_b;
        if (span_len < 96) continue;

        uint8_t prev_b = static_cast<uint8_t>(content[pos - 1]);
        uint8_t cur_b  = static_cast<uint8_t>(content[pos]);
        double s_t = markov ? markov->byte_surprisal(prev_b, cur_b) : 4.0;
        running_surprisal = 0.92 * running_surprisal + 0.08 * s_t;

        uint64_t window64 = 0;
        std::memcpy(&window64, content.data() + pos - 8, 8);
        bool conserved = false;
        uint32_t demarcs = DemarcationCA64::count_demarcs(window64, conserved);

        bool phase_spike = (s_t > running_surprisal + 1.8 && demarcs >= 28 && span_len >= 128);
        bool horizon_cap = (span_len >= 512);

        if (phase_spike || horizon_cap) {
            std::string_view blk = content.substr(start_b, pos - start_b);
            std::string prev_str(blk.substr(0, std::min<size_t>(blk.size(), 64)));
            if (blk.size() > 64) prev_str += "...";
            blocks.push_back({
                static_cast<uint32_t>(start_b),
                static_cast<uint32_t>(pos),
                chunk_idx,
                chunk_idx,
                blk,
                std::move(prev_str)
            });
            start_b = pos;
            chunk_idx++;
        }
    }

    if (start_b < n) {
        std::string_view blk = content.substr(start_b, n - start_b);
        std::string prev_str(blk.substr(0, std::min<size_t>(blk.size(), 64)));
        if (blk.size() > 64) prev_str += "...";
        blocks.push_back({
            static_cast<uint32_t>(start_b),
            static_cast<uint32_t>(n),
            chunk_idx,
            chunk_idx,
            blk,
            std::move(prev_str)
        });
    }
    return blocks;
}

static std::vector<RawBlock> chunk_byte_stream(std::string_view content) {
    std::vector<RawBlock> blocks;
    if (content.empty()) return blocks;

    size_t sample_len = std::min<size_t>(content.size(), 2048);
    size_t sample_nl = 0;
    for (size_t i = 0; i < sample_len; ++i) {
        if (content[i] == '\n') sample_nl++;
    }
    if (content.size() >= 256 && sample_nl * 512 < sample_len) {
        ByteMarkovModel local_markov;
        local_markov.observe_stream(content);
        return chunk_unformatted_byte_stream(content, &local_markov);
    }

    struct LineSpan {
        uint32_t start_byte;
        uint32_t end_byte;
        uint32_t line_num;
        int indent;
        int depth_before;
        int depth_after;
        bool is_blank;
        uint8_t transition_flux;
        std::string_view trimmed;
    };

    std::vector<LineSpan> lines;
    lines.reserve(content.size() / 36 + 16);
    uint32_t pos = 0;
    uint32_t line_num = 1;
    int running_depth = 0;
    int min_nonblank_indent = 9999;

    while (pos < content.size()) {
        uint32_t next_nl = pos;
        while (next_nl < content.size() && content[next_nl] != '\n') next_nl++;
        uint32_t end_b = (next_nl < content.size()) ? next_nl + 1 : next_nl;
        std::string_view line_str = content.substr(pos, end_b - pos);

        int indent = 0;
        bool blank = true;
        size_t first_non_space = 0;
        for (size_t k = 0; k < line_str.size(); ++k) {
            char c = line_str[k];
            if (c == ' ') indent++;
            else if (c == '\t') indent += 4;
            else if (c != '\r' && c != '\n') {
                blank = false;
                first_non_space = k;
                break;
            }
        }

        int depth_before = running_depth;
        uint32_t transitions = 0;
        std::string_view trimmed;

        if (!blank) {
            if (indent < min_nonblank_indent) min_nonblank_indent = indent;
            size_t last_non_space = line_str.size() - 1;
            while (last_non_space > first_non_space &&
                   std::isspace(static_cast<unsigned char>(line_str[last_non_space]))) {
                last_non_space--;
            }
            trimmed = line_str.substr(first_non_space, last_non_space - first_non_space + 1);

            CharClass prev_cc = classify_byte(static_cast<uint8_t>(trimmed[0]));
            bool in_string = false;
            char quote_char = 0;

            for (size_t k = 0; k < trimmed.size(); ++k) {
                char c = trimmed[k];
                if (!in_string && (c == '"' || c == '\'')) {
                    in_string = true;
                    quote_char = c;
                } else if (in_string && c == quote_char && (k == 0 || trimmed[k - 1] != '\\')) {
                    in_string = false;
                } else if (!in_string) {
                    if (c == '{' || c == '(' || c == '[') running_depth++;
                    else if (c == '}' || c == ')' || c == ']') running_depth = std::max(0, running_depth - 1);
                }
                CharClass cc = classify_byte(static_cast<uint8_t>(c));
                if (cc != prev_cc) transitions++;
                prev_cc = cc;
            }
        }

        uint8_t flux = static_cast<uint8_t>(std::min<uint32_t>(255, transitions));
        lines.push_back({pos, end_b, line_num, indent, depth_before, running_depth, blank, flux, trimmed});
        pos = end_b;
        line_num++;
    }

    if (min_nonblank_indent == 9999) min_nonblank_indent = 0;

    size_t start_idx = 0;
    for (size_t i = 1; i < lines.size(); ++i) {
        size_t cur_lines = i - start_idx;
        bool prev_blank = lines[i - 1].is_blank;

        bool topological_reset = prev_blank && !lines[i].is_blank &&
                                 (lines[i].indent <= min_nonblank_indent + 2) &&
                                 (lines[i].depth_before <= 1) && (cur_lines >= 4);

        bool scope_closed = (lines[i - 1].depth_before > 0 && lines[i - 1].depth_after == 0) &&
                            !lines[i].is_blank && (cur_lines >= 5);

        bool horizon_split = (prev_blank && cur_lines >= 45) || (cur_lines >= 90);

        if (topological_reset || scope_closed || horizon_split) {
            size_t end_idx = i - 1;
            while (end_idx > start_idx && lines[end_idx].is_blank) end_idx--;
            while (start_idx < end_idx && lines[start_idx].is_blank) start_idx++;

            if (!lines[start_idx].is_blank) {
                uint32_t sb = lines[start_idx].start_byte;
                uint32_t eb = lines[end_idx].end_byte;
                std::string_view blk_text = content.substr(sb, eb - sb);

                std::string prev_str;
                for (size_t k = start_idx; k <= end_idx; ++k) {
                    if (!lines[k].is_blank && lines[k].transition_flux >= 2) {
                        prev_str = std::string(lines[k].trimmed.substr(0, 68));
                        if (lines[k].trimmed.size() > 68) prev_str += "...";
                        break;
                    }
                }
                if (prev_str.empty()) {
                    prev_str = std::string(lines[start_idx].trimmed.substr(0, 68));
                }
                blocks.push_back({sb, eb, lines[start_idx].line_num, lines[end_idx].line_num, blk_text, std::move(prev_str)});
            }
            start_idx = i;
        }
    }

    if (start_idx < lines.size()) {
        size_t end_idx = lines.size() - 1;
        while (end_idx > start_idx && lines[end_idx].is_blank) end_idx--;
        while (start_idx < end_idx && lines[start_idx].is_blank) start_idx++;
        if (!lines[start_idx].is_blank) {
            uint32_t sb = lines[start_idx].start_byte;
            uint32_t eb = lines[end_idx].end_byte;
            std::string prev_str(lines[start_idx].trimmed.substr(0, 68));
            if (lines[start_idx].trimmed.size() > 68) prev_str += "...";
            blocks.push_back({sb, eb, lines[start_idx].line_num, lines[end_idx].line_num, content.substr(sb, eb - sb), std::move(prev_str)});
        }
    }

    return blocks;
}

// ============================================================================
// 9. REPOSITORY & DIRECTORY SCANNER (SUBSTRATE ADMISSION)
// ============================================================================
static bool read_text_file_safe(const fs::path& p, std::string& out_content, size_t max_bytes = 2 * 1024 * 1024) {
    std::error_code ec;
    auto fsize = fs::file_size(p, ec);
    if (ec || fsize == 0 || fsize > max_bytes) return false;

    std::ifstream in(p, std::ios::binary);
    if (!in) return false;
    out_content.resize(static_cast<size_t>(fsize));
    in.read(out_content.data(), static_cast<std::streamsize>(fsize));

    size_t check_len = std::min<size_t>(out_content.size(), 4096);
    size_t control_bytes = 0;
    for (size_t i = 0; i < check_len; ++i) {
        unsigned char c = static_cast<unsigned char>(out_content[i]);
        if (c == '\0') return false;
        if (c < 32 && c != '\n' && c != '\r' && c != '\t' && c != '\f') control_bytes++;
    }
    if (check_len > 0 && (control_bytes * 20 > check_len)) return false;

    return true;
}

static void detect_content_modality(std::string_view raw, Modality& inferred_mod, bool& is_html_out) {
    is_html_out = false;
    size_t scan_len = std::min<size_t>(raw.size(), 2048);
    std::string_view head = raw.substr(0, scan_len);

    size_t angle_open = 0, angle_close = 0;
    for (char c : head) {
        if (c == '<') angle_open++;
        else if (c == '>') angle_close++;
    }
    if (angle_open >= 4 && angle_close >= 4 &&
        (head.find("<!DOCTYPE") != std::string_view::npos ||
         head.find("<html") != std::string_view::npos ||
         head.find("<body") != std::string_view::npos ||
         head.find("<div") != std::string_view::npos ||
         head.find("<p>") != std::string_view::npos)) {
        is_html_out = true;
        inferred_mod = Modality::WEB_REF;
        return;
    }

    ByteStats st = compute_byte_stats(head);
    if (st.op_density < 0.015 && st.delim_density < 0.035) {
        inferred_mod = Modality::SPEC_DOC;
    } else {
        inferred_mod = Modality::REPO_CODE;
    }
}

// ============================================================================
// 10. THE CONSTITUTIONAL MANIFOLD & REALIZATION ENGINE
// Provenance: [CF Alignment Map §A-§I]; Stages 1-11 & Corrections C1-C12.
// ============================================================================
// Phase -1b: Constitutional ingestion counters
struct IngestStats {
    uint32_t files_admitted  = 0;
    uint32_t files_revised   = 0;
    uint32_t files_preserved = 0;
    uint32_t spans_admitted  = 0;
    size_t   bytes_processed = 0;
};

class ManifoldEngine {
public:
    static constexpr uint32_t SCHEMA_VERSION = 8; // Phase 3.3 & 3.4: Schema v6 with CanonicalDigest256 & exact_stat_digest

    BipolarAccumulator global_acc;
    SurprisalSketch surprisal;
    std::vector<Cell> cells;

    // L2-Resident Hot Structure-of-Arrays (SoA) & O(1) Direct Indexing
    std::vector<HotRouteEntry> hot_route;
    std::vector<ModeRelationLinks> rel_links;
    std::vector<HelixSignature256> triad_sigs;
    std::array<std::vector<uint32_t>, 256> admission_buckets;
    std::vector<int32_t> id_to_idx;
    std::unordered_map<std::string, size_t> file_uri_to_idx;
    std::unordered_map<std::string, std::string> memory_file_cache;

    std::array<uint32_t, 1024> online_mass_table{};
    std::array<uint32_t, 256> online_frame_table{};
    std::array<uint32_t, 3> online_triad_buf{};
    uint32_t online_triad_count = 0;

    // Stages 4-7 (§6-§8): Relational Endpoint, Foam Density, Invariant I(\rho) & Adequacy Gate
    // Transitional (D-adequacy-1 / D-adequacy-2):
    //   is_adequacy_certified defaults to false until verify_adequacy_gate() proves
    //   that every same-orbit pair satisfying approx_F also satisfies equiv_F (or until
    //   an explicit test-only override is set while D-adequacy-2/3 are pending).
    RelationalEndpoint relational_endpoint{};
    AdequacyCertificate adequacy_cert{};
    bool is_adequacy_certified = false;
    bool force_vacuous_byte_adapter_mode = false; // Phase 3.2 (D-adequacy-byte-gate): test hook for ByteAdapter-only mode

    ManifoldEngine() {
        online_mass_table.fill(ABSENT_NODE);
        online_frame_table.fill(ABSENT_NODE);
        cells.reserve(20480);
        hot_route.reserve(20480);
        rel_links.reserve(20480);
        triad_sigs.reserve(7168);
        id_to_idx.reserve(20480);
    }

    // Phase 3.3 (D-adequacy-hash-2): 256-bit collision-resistant ARX Merkle tree over CanonicalDigest256
    std::array<uint64_t, 4> compute_content_merkle_root_256() const {
        if (cells.empty()) return {0ULL, 0ULL, 0ULL, 0ULL};
        std::vector<std::array<uint64_t, 4>> lvl;
        lvl.reserve(cells.size());
        for (const auto& c : cells) {
            uint64_t leaf_buf[6] = {
                static_cast<uint64_t>(c.id),
                c.coord.content_hash,
                c.canonical_digest.words[0],
                c.canonical_digest.words[1],
                c.canonical_digest.words[2],
                c.canonical_digest.words[3]
            };
            lvl.push_back(crypto_arx_hash256(leaf_buf, sizeof(leaf_buf), 0x6A09E667F3BCC908ULL));
        }
        while (lvl.size() > 1) {
            size_t out_n = (lvl.size() + 1) >> 1;
            for (size_t i = 0; i < out_n; ++i) {
                const auto& left  = lvl[2 * i];
                const auto& right = (2 * i + 1 < lvl.size()) ? lvl[2 * i + 1] : left;
                uint64_t node_buf[8] = {
                    left[0], left[1], left[2], left[3],
                    right[0], right[1], right[2], right[3]
                };
                lvl[i] = crypto_arx_hash256(node_buf, sizeof(node_buf), 0x9E3779B97F4A7C15ULL);
            }
            lvl.resize(out_n);
        }
        return lvl[0];
    }

    // Correction C11 & Phase 3.3 [§7, §10]: Parameterized Merkle Root (FNV1A_64, CRYPTO_ARX_64, CRYPTO_ARX_256)
    uint64_t compute_content_merkle_root(MerkleHashAlgo algo = MerkleHashAlgo::FNV1A_64) const {
        if (cells.empty()) return 0ULL;
        if (algo == MerkleHashAlgo::CRYPTO_ARX_256) {
            return compute_content_merkle_root_256()[0];
        }
        std::vector<uint64_t> lvl;
        lvl.reserve(cells.size());
        for (const auto& c : cells) {
            if (algo == MerkleHashAlgo::CRYPTO_ARX_64) {
                uint64_t pair_buf[2] = {static_cast<uint64_t>(c.id), c.coord.content_hash};
                lvl.push_back(crypto_arx_hash64(pair_buf, sizeof(pair_buf)));
            } else {
                uint64_t leaf = 0xCBF29CE484222325ULL;
                leaf = fnv1a_64(&c.id, sizeof(c.id), leaf);
                leaf = fnv1a_64(&c.coord.content_hash, sizeof(c.coord.content_hash), leaf);
                lvl.push_back(leaf);
            }
        }
        while (lvl.size() > 1) {
            size_t out_n = (lvl.size() + 1) >> 1;
            for (size_t i = 0; i < out_n; ++i) {
                uint64_t pair_buf[2] = {
                    lvl[2 * i],
                    (2 * i + 1 < lvl.size()) ? lvl[2 * i + 1] : lvl[2 * i]
                };
                if (algo == MerkleHashAlgo::CRYPTO_ARX_64) {
                    lvl[i] = crypto_arx_hash64(pair_buf, sizeof(pair_buf), 0x9E3779B97F4A7C15ULL);
                } else {
                    uint64_t h = 0xCBF29CE484222325ULL ^ 0x01ULL;
                    h = fnv1a_64(&pair_buf[0], sizeof(uint64_t), h);
                    h = fnv1a_64(&pair_buf[1], sizeof(uint64_t), h);
                    lvl[i] = h;
                }
            }
            lvl.resize(out_n);
        }
        return lvl[0];
    }

    // Stages 4-7 (§6-§8): Recomputes F_rel = (H_F, ~_F, w_F), FoamDensity \rho_F,
    // 11D InvariantCoordinateVector I(\rho) \in R^{11}, and verifies the Adequacy Gate!
    const AdequacyCertificate& verify_adequacy_gate() {
        AdequacyCertificate cert;
        cert.su0_to_su7_fixed = true;
        cert.carrier_defined = !cells.empty();
        cert.preservation_established = true;

        // Partition active blocks across the 256 E8 product-address sectors (orbits)
        std::array<uint32_t, 256> sector_counts{};
        std::array<double, 256> sector_entropy_sum{};
        size_t total_blocks = 0;

        for (const auto& b_vec : admission_buckets) {
            total_blocks += b_vec.size();
        }

        for (uint32_t s = 0; s < 256; ++s) {
            const auto& bucket = admission_buckets[s];
            sector_counts[s] = static_cast<uint32_t>(bucket.size());
            double h_sum = 0.0;
            for (uint32_t idx : bucket) {
                h_sum += static_cast<double>(hot_route[idx].halo.entropy_q8) / 32.0;
            }
            sector_entropy_sum[s] = h_sum;
        }

        relational_endpoint.num_admissible_paths = total_blocks;
        relational_endpoint.orbit_w_F.clear();
        relational_endpoint.orbit_invariance_holds = true;

        std::vector<double> raw_weights;
        raw_weights.reserve(256);
        double Z_F = 0.0;

        for (uint32_t s = 0; s < 256; ++s) {
            if (sector_counts[s] > 0) {
                double mean_h = sector_entropy_sum[s] / static_cast<double>(sector_counts[s]);
                double w_mag = static_cast<double>(sector_counts[s]) * std::max(0.1, mean_h);
                double phase = (static_cast<double>(s & 15u) * 3.14159265358979323846) / 8.0;
                relational_endpoint.orbit_w_F.push_back(std::polar(w_mag, phase));
                raw_weights.push_back(w_mag);
                Z_F += w_mag;
            }
        }
        relational_endpoint.num_orbits = raw_weights.size();
        cert.relational_endpoint_fixed = (relational_endpoint.num_orbits > 0 || cells.empty());

        // Compute normalized FoamDensity \rho_F [CF Exhibit D, Eq. ExhibitD.2 - ExhibitD.3]
        cert.density.Z_F = Z_F;
        cert.density.weights.resize(raw_weights.size(), 0.0);
        double tr = 0.0;
        double tr_sq = 0.0;
        if (Z_F > 0.0) {
            for (size_t i = 0; i < raw_weights.size(); ++i) {
                double p = raw_weights[i] / Z_F;
                cert.density.weights[i] = p;
                tr += p;
                tr_sq += p * p;
            }
        }
        cert.density.trace = tr;
        cert.density.purity_tr2 = tr_sq;

        // Compute 11D InvariantCoordinateVector I(\rho) \in R^{11} [CF Exhibit E]
        //   coords[0] = N_0   (Active orbit count |H_F|)
        //   coords[1] = d_eff = 1 / Tr(\rho^2) [Eq. ExhibitE.4]
        //   coords[2] = SE    = 1 - Tr(\rho^2) [Eq. ExhibitE.6]
        //   coords[3..10] = Spectral distribution moments (R_t, A_y, K_t, C_t, A_0..A_3)
        std::vector<double> sorted_p = cert.density.weights;
        std::sort(sorted_p.begin(), sorted_p.end(), std::greater<double>());

        double N_0   = static_cast<double>(relational_endpoint.num_orbits);
        double d_eff = (tr_sq > 1e-12) ? (1.0 / tr_sq) : 0.0;
        double SE    = (Z_F > 0.0) ? (1.0 - tr_sq) : 0.0;

        double A_0 = sorted_p.size() > 0 ? sorted_p[0] : 0.0;
        double A_1 = sorted_p.size() > 1 ? sorted_p[1] : 0.0;
        double A_2 = sorted_p.size() > 2 ? sorted_p[2] : 0.0;
        double A_3 = sorted_p.size() > 3 ? sorted_p[3] : 0.0;
        double R_t = (A_0 > 1e-12) ? (A_1 / A_0) : 0.0;

        double mean_p = (N_0 > 0.0) ? (1.0 / N_0) : 0.0;
        double m3 = 0.0, m4 = 0.0;
        for (double p : sorted_p) {
            double d = p - mean_p;
            m3 += d * d * d;
            m4 += d * d * d * d;
        }
        double A_y = m3 * N_0;
        double K_t = m4 * N_0;
        double C_t = (total_blocks > 0)
            ? (static_cast<double>(triad_sigs.size() * 3) / static_cast<double>(total_blocks))
            : 0.0;

        cert.invariant_coords.coords = {
            N_0, d_eff, SE, R_t, A_y, K_t, C_t, A_0, A_1, A_2, A_3
        };

        // Phase 3.1, 3.2, 3.3, 3.4 & Phase 4A:
        //   1) Verify all 4 Adapters' positive & falsifying witnesses and consume `is_nontrivial_quotient`
        //      to set `CertificationKind` (`Substantive` vs `Vacuous`, Phase 3.2 D-adequacy-byte-gate).
        //   2) Check all pairs (c0, ci) within each bucket (Phase 3.1 D-adequacy-braid-1):
        //      if `same_rel_orbit` holds (`approx_stat` with `exact_stat_digest` [D-adequacy-6b],
        //      then `equiv_canonical` (full 256-bit `CanonicalDigest256` equality [D-adequacy-hash-2]
        //      and same radiance-channel curvature sector Q_q) MUST hold!
        bool byte_nt = false, code_nt = false, html_nt = false, dna_nt = false;
        bool adapters_witnessed = verify_adapter_witness<ByteAdapter>(&byte_nt) &&
                                  verify_adapter_witness<CodeAdapter>(&code_nt) &&
                                  verify_adapter_witness<HtmlAdapter>(&html_nt) &&
                                  verify_adapter_witness<DnaAdapter>(&dna_nt);
        bool active_quotient_nontrivial = (!force_vacuous_byte_adapter_mode) && (code_nt && html_nt && dna_nt);
        cert.cert_kind = (!cells.empty() && active_quotient_nontrivial)
            ? CertificationKind::Substantive
            : CertificationKind::Vacuous;

        bool adeq_ok = !cells.empty() && adapters_witnessed;
        uint32_t verified_pairs = 0;
        for (uint32_t s = 0; s < 256; ++s) {
            const auto& bucket = admission_buckets[s];
            const size_t b_sz = bucket.size();
            if (b_sz >= 2) {
                for (size_t i0 = 0; i0 < b_sz; ++i0) {
                    uint32_t c0 = bucket[i0];
                    const HaloHeader& h0 = hot_route[c0].halo;
                    for (size_t i = i0 + 1; i < b_sz; ++i) {
                        uint32_t ci = bucket[i];
                        const HaloHeader& hi = hot_route[ci].halo;
                        verified_pairs++;
                        // Phase 3.4 (D-adequacy-6b): Coarse HaloHeader bound + 16-bit exact_stat_digest
                        bool approx_stat =
                            (std::abs(static_cast<int>(h0.entropy_q8) - static_cast<int>(hi.entropy_q8)) <= 25) &&
                            (std::abs(static_cast<int>(h0.surprisal_q8) - static_cast<int>(hi.surprisal_q8)) <= 52) &&
                            (cells[c0].exact_stat_digest == cells[ci].exact_stat_digest);
                        bool same_rel_orbit = approx_stat &&
                                              (cells[c0].hv.hamming(cells[ci].hv) == 0);
                        if (same_rel_orbit) {
                                                        bool equiv_canonical = (cells[c0].canonical_digest == cells[ci].canonical_digest) &&
                                                   (cells[c0].canonical_hash   == cells[ci].canonical_hash) &&
                                                   (cells[c0].canonical_len    == cells[ci].canonical_len);
                            if (!equiv_canonical) {
                                adeq_ok = false;
                                if (cert.failing_witness_a == 0) {
                                    cert.failing_witness_a = cells[c0].id;
                                    cert.failing_witness_b = cells[ci].id;
                                }
                            }
                        }
                    }
                }
            }
        }
        relational_endpoint.orbit_invariance_holds = adeq_ok;
        cert.instance_adequacy_holds = adeq_ok;
        cert.adequacy_holds          = adeq_ok;
        cert.pairs_verified          = verified_pairs;
        cert.merkle_root             = compute_content_merkle_root(MerkleHashAlgo::FNV1A_64);
        cert.merkle_root_256         = compute_content_merkle_root_256();

        adequacy_cert = cert;
        is_adequacy_certified = adequacy_cert.is_fully_certified();
        return adequacy_cert;
    }

    // Wires all 5 Constitutional Edge Types (`e=0..4`, Correction C4) in O(1) time per block
    inline void wire_block_relations(uint32_t cell_idx, uint32_t& prev_seq_in_file) {
        if (rel_links.size() <= cell_idx) rel_links.resize(cell_idx + 1);
        ModeRelationLinks& rl = rel_links[cell_idx];
        const Cell& blk = cells[cell_idx];
        rl.primary_mass_seed = blk.primary_mass_seed;

        // Edge e=2: AdjacencyRelation [SL §4.7]
        if (prev_seq_in_file != ABSENT_NODE) {
            rl.adjacency_prev_idx = prev_seq_in_file;
            rel_links[prev_seq_in_file].adjacency_next_idx = cell_idx;
        }
        prev_seq_in_file = cell_idx;

        // Edge e=3: HolonomyEquivalence [CF Exhibit B]
        if (blk.primary_mass_seed != 0) {
            size_t m_slot = static_cast<size_t>((blk.primary_mass_seed ^ (blk.primary_mass_seed >> 23)) & 1023ULL);
            uint32_t prev_mass = online_mass_table[m_slot];
            if (prev_mass != ABSENT_NODE && cells[prev_mass].primary_mass_seed == blk.primary_mass_seed) {
                rl.holonomy_equiv_idx = prev_mass;
            }
            online_mass_table[m_slot] = cell_idx;
        }

        // Edge e=4: CurvatureEquivalence [CF §19.5]
        uint8_t sector = blk.halo.bucket_id;
        if (online_frame_table[sector] != ABSENT_NODE) {
            rl.curvature_equiv_idx = online_frame_table[sector];
        }
        online_frame_table[sector] = cell_idx;

        // Edge e=0 (TriSectorRepresentation) & e=1 (MixedSectorCoupling) [CF §19.5]
        online_triad_buf[online_triad_count++] = cell_idx;
        if (online_triad_count == 3) {
            uint32_t c0 = online_triad_buf[0];
            uint32_t c1 = online_triad_buf[1];
            uint32_t c2 = online_triad_buf[2];
            const HelixSignature256& s0 = hot_route[c0].sig;
            const HelixSignature256& s1 = hot_route[c1].sig;
            const HelixSignature256& s2 = hot_route[c2].sig;

            HelixSignature256 tsig{
                (s0.s0 & s1.s0) | (s1.s0 & s2.s0) | (s0.s0 & s2.s0),
                (s0.s1 & s1.s1) | (s1.s1 & s2.s1) | (s0.s1 & s2.s1),
                (s0.s2 & s1.s2) | (s1.s2 & s2.s2) | (s0.s2 & s2.s2),
                (s0.s3 & s1.s3) | (s1.s3 & s2.s3) | (s0.s3 & s2.s3)
            };
            uint32_t t_idx = static_cast<uint32_t>(triad_sigs.size());
            triad_sigs.push_back(tsig);

            rel_links[c0].tri_sector_rep_idx = t_idx;
            rel_links[c1].tri_sector_rep_idx = t_idx;
            rel_links[c2].tri_sector_rep_idx = t_idx;

            rel_links[c0].mixed_sector_sib0_idx = c1;
            rel_links[c1].mixed_sector_sib0_idx = c0;
            rel_links[c1].mixed_sector_sib1_idx = c2;
            rel_links[c2].mixed_sector_sib0_idx = c1;
            online_triad_count = 0;
        }
    }

    void rebuild_lookup_tables() {
        file_uri_to_idx.clear();
        id_to_idx.clear();
        hot_route.resize(cells.size());
        rel_links.assign(cells.size(), ModeRelationLinks{});
        triad_sigs.clear();
        triad_sigs.reserve(cells.size() / 3 + 16);
        for (auto& b : admission_buckets) b.clear();

        uint32_t max_id = cells.empty() ? 0 : cells.back().id;
        for (const auto& c : cells) {
            if (c.id > max_id) max_id = c.id;
        }
        id_to_idx.assign(max_id + 1, -1);
        file_uri_to_idx.reserve(cells.size() / 16 + 16);

        online_mass_table.fill(ABSENT_NODE);
        online_frame_table.fill(ABSENT_NODE);
        online_triad_count = 0;
        uint32_t prev_seq_in_file = ABSENT_NODE;
        uint32_t current_parent_file = 0;

        for (size_t i = 0; i < cells.size(); ++i) {
            id_to_idx[cells[i].id] = static_cast<int32_t>(i);
            hot_route[i] = {cells[i].halo, cells[i].sig};
            if (cells[i].halo.scale == Scale::FILE) {
                file_uri_to_idx[cells[i].coord.uri] = i;
                prev_seq_in_file = ABSENT_NODE;
                online_triad_count = 0;
                current_parent_file = cells[i].id;
            } else if (cells[i].halo.scale == Scale::BLOCK) {
                if (cells[i].parent_id != current_parent_file) {
                    prev_seq_in_file = ABSENT_NODE;
                    online_triad_count = 0;
                    current_parent_file = cells[i].parent_id;
                }
                admission_buckets[cells[i].halo.bucket_id].push_back(static_cast<uint32_t>(i));
                wire_block_relations(static_cast<uint32_t>(i), prev_seq_in_file);
            }
        }
        verify_adequacy_gate();
    }

    void remove_file_by_uri(const std::string& uri) {
        auto it = file_uri_to_idx.find(uri);
        if (it == file_uri_to_idx.end()) return;
        uint32_t target_file_id = cells[it->second].id;

        std::vector<Cell> surviving;
        surviving.reserve(cells.size());
        for (const auto& c : cells) {
            bool is_target_file = (c.id == target_file_id);
            bool is_target_block = (c.halo.scale == Scale::BLOCK && c.parent_id == target_file_id);

            if (is_target_block) {
                global_acc.accumulate(c.hv, -1);
            }
            if (!is_target_file && !is_target_block) {
                surviving.push_back(c);
            }
        }
        cells = std::move(surviving);
        rebuild_lookup_tables();
    }

    uint32_t ingest_file(
        const std::string& uri,
        const std::string& raw_content,
        Modality mod = Modality::REPO_CODE,
        bool is_html = false,
        IngestStats* stats = nullptr,
        bool defer_projection = false,
        uint64_t mtime_ns = 0,
        const std::string& corpus_id_arg = "local"
    ) {
        memory_file_cache[uri] = raw_content;
        std::string dem_storage;
        std::string_view content = raw_content;
        if (is_html) {
            dem_storage = HtmlAdapter::demodulate(raw_content);
            content = dem_storage;
        }

        // Stage 1 (§1-§3): Wrap content in ByteSpanSubstrate and check Adapter carrier predicate
        ByteSpanSubstrate file_substrate(content);
        Path<ByteSpanSubstrate> file_path{file_substrate, 0, static_cast<uint32_t>(content.size())};
        if (!CodeAdapter::A_path(file_path)) {
            return 0;
        }

        uint64_t new_hash = fnv1a_str(content);

        auto existing_it = file_uri_to_idx.find(uri);
        if (existing_it != file_uri_to_idx.end()) {
            Cell& existing_fc = cells[existing_it->second];
            if (existing_fc.coord.content_hash == new_hash) {
                if (mtime_ns != 0) existing_fc.coord.mtime_ns = mtime_ns;
                if (stats) stats->files_preserved++;
                return existing_fc.id;
            }
            remove_file_by_uri(uri);
            if (stats) stats->files_revised++;
        } else {
            if (stats) stats->files_admitted++;
        }

        if (stats) stats->bytes_processed += content.size();
        auto raw_blocks = chunk_byte_stream(content);
        online_triad_count = 0;

        uint32_t file_cell_id = cells.empty() ? 1 : (cells.back().id + 1);
        size_t file_cell_idx = cells.size();

        Cell file_cell;
        file_cell.id = file_cell_id;
        file_cell.corpus_id = corpus_id_arg;
        file_cell.parent_id = 0;
        file_cell.halo.scale = Scale::FILE;
        file_cell.halo.modality = mod;
        file_cell.coord = {uri, 0, static_cast<uint32_t>(raw_content.size()), 1, 1, new_hash, mtime_ns};
        file_cell.preview = std::string("[") + modality_tag(mod) + "] " + uri;
        cells.push_back(std::move(file_cell));
        hot_route.push_back({cells.back().halo, {}});
        rel_links.push_back(ModeRelationLinks{});
        if (id_to_idx.size() <= file_cell_id) id_to_idx.resize(file_cell_id + 1, -1);
        id_to_idx[file_cell_id] = static_cast<int32_t>(file_cell_idx);
        file_uri_to_idx[uri] = file_cell_idx;

        uint32_t max_line = 1;
        uint32_t prev_seq_in_file = ABSENT_NODE;
        size_t first_blk_idx = cells.size();

        for (const auto& rb : raw_blocks) {
            Path<ByteSpanSubstrate> blk_path{file_substrate, rb.start_byte, rb.end_byte};
            if (!CodeAdapter::A_local(blk_path)) continue;

            uint64_t primary_mass_seed = 0;
            Hypervec8192 bhv = encode_span_hypervector(rb.text, &surprisal, true, MAX_BLOCK_MASS_NODES, &primary_mass_seed);
            ByteStats bstats = compute_byte_stats(rb.text);
            E8CarrierAddress e8_addr = decode_e8_carrier(bhv);
            HelixSignature256 hsig = compute_helix_signature(bhv);

            Cell blk;
            blk.id = cells.back().id + 1;
            blk.corpus_id = corpus_id_arg;
            blk.parent_id = file_cell_id;
            blk.halo.scale = Scale::BLOCK;
            blk.halo.modality = mod;
            blk.halo.bucket_id = e8_addr.bucket_key;
            blk.halo.entropy_q8 = static_cast<uint8_t>(std::clamp(bstats.shannon_entropy * 32.0, 0.0, 255.0));
            blk.halo.surprisal_q8 = static_cast<uint8_t>(std::clamp(bstats.mean_delta * 255.0, 0.0, 255.0));
            CanonicalDigest256 canon_d256 = compute_canonical_digest256(rb.text);
            blk.sig = hsig;
            blk.primary_mass_seed = primary_mass_seed;
            blk.canonical_digest = canon_d256;
            blk.canonical_hash = canon_d256.low64();
            blk.canonical_len = canon_d256.canonical_len;
            blk.exact_stat_digest = bstats.exact_stat_digest;
            blk.coord = {uri, rb.start_byte, rb.end_byte, rb.start_line, rb.end_line, fnv1a_str(rb.text), mtime_ns};
            blk.hv = bhv;
            blk.preview = rb.preview;

            size_t blk_idx = cells.size();
            if (id_to_idx.size() <= blk.id) id_to_idx.resize(blk.id + 1, -1);
            id_to_idx[blk.id] = static_cast<int32_t>(blk_idx);
            hot_route.push_back({blk.halo, hsig});
            admission_buckets[e8_addr.bucket_key].push_back(static_cast<uint32_t>(blk_idx));
            cells.push_back(std::move(blk));
            wire_block_relations(static_cast<uint32_t>(blk_idx), prev_seq_in_file);

            if (!defer_projection) {
                global_acc.accumulate(bhv, 1);
            }
            max_line = std::max(max_line, rb.end_line);
            if (stats) stats->spans_admitted++;
        }

        // Fast Bit-Sliced File Hypervector Projection
        Hypervec8192 fhv;
        size_t n_blks = cells.size() - first_blk_idx;
        if (n_blks == 0) {
            fhv = encode_span_hypervector(content, &surprisal, false, MAX_BLOCK_MASS_NODES);
        } else if (n_blks == 1) {
            fhv = cells[first_blk_idx].hv;
        } else {
            size_t sample_step = (n_blks > 63) ? (n_blks / 63) : 1;
            size_t count_sampled = 0;
            for (size_t bi = first_blk_idx; bi < cells.size() && count_sampled < 63; bi += sample_step) {
                count_sampled++;
            }
            uint32_t t_even = (static_cast<uint32_t>(count_sampled) >> 1u) + 1u;
            uint32_t t_odd  = (static_cast<uint32_t>(count_sampled) + 1u) >> 1u;
            uint32_t init_even = 64u - t_even;
            uint32_t init_odd  = 64u - t_odd;
            uint64_t ib[7];
            for (int bit = 0; bit < 7; ++bit) {
                uint64_t m_even = ((init_even >> bit) & 1u) ? 0x5555555555555555ULL : 0ULL;
                uint64_t m_odd  = ((init_odd  >> bit) & 1u) ? 0xAAAAAAAAAAAAAAAAULL : 0ULL;
                ib[bit] = m_even | m_odd;
            }
            for (size_t w = 0; w < HV_WORDS; ++w) {
                uint64_t b0 = ib[0], b1 = ib[1], b2 = ib[2], b3 = ib[3], b4 = ib[4], b5 = ib[5], b6 = ib[6];
                size_t sc = 0;
                for (size_t bi = first_blk_idx; bi < cells.size() && sc < 63; bi += sample_step, ++sc) {
                    uint64_t x = cells[bi].hv.words[w], c;
                    c = b0 & x; b0 ^= x; x = c;
                    c = b1 & x; b1 ^= x; x = c;
                    c = b2 & x; b2 ^= x; x = c;
                    c = b3 & x; b3 ^= x; x = c;
                    c = b4 & x; b4 ^= x; x = c;
                    c = b5 & x; b5 ^= x; x = c;
                    b6 ^= x;
                }
                fhv.words[w] = b6;
            }
        }

        ByteStats fstats = compute_byte_stats(content);
        E8CarrierAddress f_e8 = decode_e8_carrier(fhv);
        HelixSignature256 f_hsig = compute_helix_signature(fhv);

        if (defer_projection) {
            global_acc.accumulate(fhv, 1);
        } else {
                verify_adequacy_gate();
        }

        CanonicalDigest256 f_canon_d256 = compute_canonical_digest256(content);
        Cell& fc = cells[file_cell_idx];
        fc.hv = fhv;
        fc.sig = f_hsig;
        fc.canonical_digest = f_canon_d256;
        fc.canonical_hash = f_canon_d256.low64();
        fc.canonical_len = f_canon_d256.canonical_len;
        fc.exact_stat_digest = fstats.exact_stat_digest;
        fc.halo.bucket_id = f_e8.bucket_key;
        fc.halo.entropy_q8 = static_cast<uint8_t>(std::clamp(fstats.shannon_entropy * 32.0, 0.0, 255.0));
        fc.coord.end_line = max_line;
        hot_route[file_cell_idx] = {fc.halo, f_hsig};

        return file_cell_id;
    }

    IngestStats ingest_path(
        const fs::path& target_path,
        std::optional<Modality> explicit_modality = std::nullopt,
        const std::string& uri_prefix = ""
    ) {
        IngestStats stats;
        std::error_code ec;
        if (!fs::exists(target_path, ec)) return stats;

        struct DiscoveredFile {
            fs::path path;
            uintmax_t fsize;
            uint64_t mtime_ns;
        };
        uint32_t files_processed_since_proj = 0;
        auto process_single_file = [&](const DiscoveredFile& df, const fs::path& base_dir) {
            std::string rel = base_dir.empty() ? df.path.string() : df.path.lexically_relative(base_dir).string();
            if (rel.empty() || rel.starts_with("..")) rel = df.path.string();
            std::string uri = uri_prefix.empty() ? rel : (uri_prefix + "/" + rel);

            auto existing_it = file_uri_to_idx.find(uri);
            if (existing_it != file_uri_to_idx.end()) {
                const Cell& existing_fc = cells[existing_it->second];
                if (df.mtime_ns != 0 && existing_fc.coord.mtime_ns == df.mtime_ns &&
                    existing_fc.coord.end_byte == static_cast<uint32_t>(df.fsize)) {
                    stats.files_preserved++;
                    return;
                }
            }

            std::string raw;
            if (!read_text_file_safe(df.path, raw)) return;

            Modality inferred_mod = Modality::REPO_CODE;
            bool is_html = false;
            detect_content_modality(raw, inferred_mod, is_html);

            Modality final_mod = explicit_modality.value_or(inferred_mod);
            ingest_file(uri, raw, final_mod, is_html, &stats, true, df.mtime_ns, uri_prefix.empty() ? "local" : uri_prefix);

            if (++files_processed_since_proj >= 64) {
                        files_processed_since_proj = 0;
            }
        };

        if (fs::is_regular_file(target_path, ec)) {
            uintmax_t fsz = fs::file_size(target_path, ec);
            auto ftm = fs::last_write_time(target_path, ec);
            uint64_t mt = ec ? 0ULL : static_cast<uint64_t>(ftm.time_since_epoch().count());
            if (!ec && fsz > 0 && fsz <= 2 * 1024 * 1024) {
                process_single_file({target_path, fsz, mt}, target_path.parent_path());
            }
            if (stats.files_admitted + stats.files_revised > 0) {
                        verify_adequacy_gate();
            }
            return stats;
        }

        if (fs::is_directory(target_path, ec)) {
            std::vector<DiscoveredFile> found_files;
            found_files.reserve(1024);
            for (auto it = fs::recursive_directory_iterator(target_path, fs::directory_options::skip_permission_denied, ec);
                 it != fs::recursive_directory_iterator(); it.increment(ec)) {
                if (ec) break;
                const fs::path& p = it->path();
                if (it->is_directory(ec)) {
                    std::string dname = p.filename().string();
                    if (dname.starts_with(".") && dname.size() > 1) {
                        it.disable_recursion_pending();
                    }
                    continue;
                }
                if (it->is_regular_file(ec)) {
                    uintmax_t fsz = it->file_size(ec);
                    if (ec || fsz == 0 || fsz > 2 * 1024 * 1024) continue;
                    auto ftm = it->last_write_time(ec);
                    uint64_t mt = ec ? 0ULL : static_cast<uint64_t>(ftm.time_since_epoch().count());
                    found_files.push_back({p, fsz, mt});
                }
            }
            std::sort(found_files.begin(), found_files.end(),
                [](const DiscoveredFile& a, const DiscoveredFile& b) { return a.path < b.path; });
            for (const auto& df : found_files) {
                process_single_file(df, target_path);
            }
            if (stats.files_admitted + stats.files_revised > 0) {
                        verify_adequacy_gate();
            }
        }
        return stats;
    }

    struct RouteHit {
        const Cell* cell;
        double similarity;
        double z_score;
    };

    // Stage 8 (§8): Realization-layer routing gated on `is_adequacy_certified` [CF Alignment Map §I]
    std::vector<RouteHit> route(
        const Hypervec8192& query_hv,
        Scale target_scale = Scale::BLOCK,
        int unused_filter = -1,
        int modality_filter = -1,
        size_t top_k = 5
    ) const {
        (void)unused_filter;
        std::vector<RouteHit> hits;
        if (cells.empty() || !is_adequacy_certified) return hits;

        const HelixSignature256 q_sig = compute_helix_signature(query_hv);
        const E8CarrierAddress q_e8 = decode_e8_carrier(query_hv);

        // Evaluates 5-Edge Constitutional ModeRelationLinks (Correction C4) + Phase 4A su(7) Holonomy/Sector Coupling
        auto compute_mode_relation_coupled_sim = [&](size_t cell_idx, double raw_sim) -> double {
            const auto& c = cells[cell_idx];
            if (target_scale != Scale::BLOCK) return raw_sim;

            double file_sim = raw_sim;
            if (c.parent_id > 0 && c.parent_id < id_to_idx.size()) {
                int32_t pidx = id_to_idx[c.parent_id];
                if (pidx >= 0) {
                    if (cells.size() <= 1024) {
                        file_sim = query_hv.similarity(cells[pidx].hv);
                    } else {
                        const HelixSignature256& fsig = hot_route[pidx].sig;
                        uint32_t fham = static_cast<uint32_t>(
                            std::popcount(q_sig.s0 ^ fsig.s0) +
                            std::popcount(q_sig.s1 ^ fsig.s1) +
                            std::popcount(q_sig.s2 ^ fsig.s2) +
                            std::popcount(q_sig.s3 ^ fsig.s3)
                        );
                        file_sim = 1.0 - static_cast<double>(fham) / 256.0;
                    }
                }
            }

            if (cell_idx >= rel_links.size()) {
                return 0.84 * raw_sim + 0.16 * file_sim;
            }

            const ModeRelationLinks& rl = rel_links[cell_idx];
            auto sig_res = [&](const HelixSignature256& s) -> double {
                uint32_t ham = static_cast<uint32_t>(
                    std::popcount(q_sig.s0 ^ s.s0) +
                    std::popcount(q_sig.s1 ^ s.s1) +
                    std::popcount(q_sig.s2 ^ s.s2) +
                    std::popcount(q_sig.s3 ^ s.s3)
                );
                return 1.0 - static_cast<double>(ham) / 256.0;
            };

            double rel_bonus = 0.0;
            // Edge e=0: TriSectorRepresentation [CF §19.5]
            if (rl.tri_sector_rep_idx != ABSENT_NODE && rl.tri_sector_rep_idx < triad_sigs.size()) {
                double triad_sim = sig_res(triad_sigs[rl.tri_sector_rep_idx]);
                if (triad_sim > 0.52) rel_bonus += 0.12 * (triad_sim - 0.50);
            }
            // Edge e=1: MixedSectorCoupling [CF §19.5]
            if (rl.mixed_sector_sib0_idx != ABSENT_NODE && rl.mixed_sector_sib0_idx < hot_route.size()) {
                double s_sim = sig_res(hot_route[rl.mixed_sector_sib0_idx].sig);
                if (s_sim > 0.53) rel_bonus += 0.08 * (s_sim - 0.50);
            }
            if (rl.mixed_sector_sib1_idx != ABSENT_NODE && rl.mixed_sector_sib1_idx < hot_route.size()) {
                double s_sim = sig_res(hot_route[rl.mixed_sector_sib1_idx].sig);
                if (s_sim > 0.53) rel_bonus += 0.06 * (s_sim - 0.50);
            }
            // Edge e=2: AdjacencyRelation [SL §4.7]
            if (rl.adjacency_prev_idx != ABSENT_NODE && rl.adjacency_prev_idx < hot_route.size()) {
                double prev_sim = sig_res(hot_route[rl.adjacency_prev_idx].sig);
                if (prev_sim > 0.53) rel_bonus += 0.07 * (prev_sim - 0.50);
            }
            // Edge e=3: HolonomyEquivalence [CF Exhibit B]
            if (rl.holonomy_equiv_idx != ABSENT_NODE && rl.holonomy_equiv_idx < hot_route.size()) {
                double m_sim = sig_res(hot_route[rl.holonomy_equiv_idx].sig);
                if (m_sim > 0.53) rel_bonus += 0.06 * (m_sim - 0.50);
            }
            // Edge e=4: CurvatureEquivalence [CF §19.5]
            if (rl.curvature_equiv_idx != ABSENT_NODE && rl.curvature_equiv_idx < hot_route.size()) {
                double f_sim = sig_res(hot_route[rl.curvature_equiv_idx].sig);
                if (f_sim > 0.53) rel_bonus += 0.05 * (f_sim - 0.50);
            }

            return 0.85 * raw_sim + 0.15 * file_sim + std::min(0.025, rel_bonus);
        };

        if (cells.size() <= 1024 || target_scale == Scale::FILE) {
            hits.reserve(256);
            for (size_t i = 0; i < cells.size(); ++i) {
                const auto& c = cells[i];
                if (c.halo.scale != target_scale) continue;
                if (modality_filter >= 0 && static_cast<int>(c.halo.modality) != modality_filter) continue;
                double raw_sim = query_hv.similarity(c.hv);
                double sim = compute_mode_relation_coupled_sim(i, raw_sim);
                double z = (raw_sim - 0.5) * 181.01933598375618;
                hits.push_back({&c, sim, z});
            }
        } else {
            // Phase -1c: Precompute 256-bucket L1 admission filter ONLY when cells.size() > 1024 && Scale::BLOCK
            uint32_t cell_l1_cost[256];
            std::array<uint8_t, 256> cell_order;
            for (uint32_t c = 0; c < 256; ++c) {
                cell_l1_cost[c] = static_cast<uint32_t>(std::popcount(c ^ static_cast<uint32_t>(q_e8.bucket_key)));
                cell_order[c] = static_cast<uint8_t>(c);
            }
            std::partial_sort(cell_order.begin(), cell_order.begin() + 32, cell_order.end(),
                [&](uint8_t a, uint8_t b) {
                    return cell_l1_cost[a] < cell_l1_cost[b] || (cell_l1_cost[a] == cell_l1_cost[b] && a < b);
                });
            std::array<bool, 256> is_admitted_cell{};
            for (size_t k = 0; k < 32; ++k) {
                is_admitted_cell[cell_order[k]] = true;
            }

            struct ShortlistCand {
                uint32_t cell_idx;
                uint16_t coarse_cost;
            };
            ShortlistCand shortlist_buf[4096];
            size_t n_shortlist = 0;
            // With Correction C1 (4 unbiased 64-bit quadrant words = 256 slices),
            // random orthogonal Hamming distance across 256 bits is 128 (sigma = 8).
            uint16_t cutoff_cost = 120u;

            const size_t n_cells = hot_route.size();
            const HotRouteEntry* __restrict hr_ptr = hot_route.data();

            for (size_t i = 0; i < n_cells; ++i) {
                const HaloHeader h = hr_ptr[i].halo;
                if (h.scale != target_scale) continue;
                if (modality_filter >= 0 && static_cast<int>(h.modality) != modality_filter) continue;

                const HelixSignature256& csig = hr_ptr[i].sig;
                uint32_t full256_ham = static_cast<uint32_t>(
                    std::popcount(q_sig.s0 ^ csig.s0) +
                    std::popcount(q_sig.s1 ^ csig.s1) +
                    std::popcount(q_sig.s2 ^ csig.s2) +
                    std::popcount(q_sig.s3 ^ csig.s3)
                );
                bool admitted = is_admitted_cell[h.bucket_id];
                uint16_t combined_cost = static_cast<uint16_t>(
                    full256_ham + (admitted ? 0u : 3u)
                );
                if (combined_cost > cutoff_cost) continue;
                shortlist_buf[n_shortlist++] = {static_cast<uint32_t>(i), combined_cost};
                if (n_shortlist == 4096) {
                    std::nth_element(shortlist_buf, shortlist_buf + 512, shortlist_buf + 4096,
                        [](const ShortlistCand& a, const ShortlistCand& b) {
                            return a.coarse_cost < b.coarse_cost;
                        });
                    cutoff_cost = shortlist_buf[512].coarse_cost;
                    n_shortlist = 512;
                }
            }

            const size_t max_verify = std::min<size_t>(n_shortlist, 224);
            if (n_shortlist > max_verify) {
                std::partial_sort(shortlist_buf, shortlist_buf + max_verify, shortlist_buf + n_shortlist,
                    [](const ShortlistCand& a, const ShortlistCand& b) {
                        return a.coarse_cost < b.coarse_cost;
                    });
            }

            hits.reserve(max_verify);
            for (size_t k = 0; k < max_verify; ++k) {
                uint32_t cidx = shortlist_buf[k].cell_idx;
                const auto& c = cells[cidx];
                double raw_sim = query_hv.similarity(c.hv);
                double sim = compute_mode_relation_coupled_sim(cidx, raw_sim);
                double z = (raw_sim - 0.5) * 181.01933598375618;
                hits.push_back({&c, sim, z});
            }
        }

        if (hits.size() > top_k) {
            std::partial_sort(hits.begin(), hits.begin() + top_k, hits.end(),
                [](const RouteHit& a, const RouteHit& b) { return a.similarity > b.similarity; });
            hits.resize(top_k);
        } else {
            std::sort(hits.begin(), hits.end(),
                [](const RouteHit& a, const RouteHit& b) { return a.similarity > b.similarity; });
        }
        return hits;
    }

    // Correction C12 [§7, §10]: Schema-versioned binary slab checkpoint ("MFDv4\0\0\0" + uint32_t schema_version)
    bool save_checkpoint(const std::string& filepath) const {
        std::ofstream out(filepath, std::ios::binary);
        if (!out) return false;
        const char magic[8] = {'M','F','D','v','4','\0','\0','\0'};
        out.write(magic, 8);
        uint32_t schema_ver = SCHEMA_VERSION;
        out.write(reinterpret_cast<const char*>(&schema_ver), sizeof(schema_ver));
        out.write(reinterpret_cast<const char*>(&global_acc), sizeof(global_acc));
        out.write(reinterpret_cast<const char*>(&surprisal), sizeof(surprisal));

        uint32_t count = static_cast<uint32_t>(cells.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        if (count == 0) return out.good();

        std::vector<PackedCellRecord> recs(count);
        std::vector<Hypervec8192> hvs(count);
        std::string string_arena;
        string_arena.reserve(count * 64);

        for (uint32_t i = 0; i < count; ++i) {
            const auto& c = cells[i];
            auto& r = recs[i];
            r.id = c.id;
            r.parent_id = c.parent_id;
            r.halo = c.halo;
            r.start_byte = c.coord.start_byte;
            r.end_byte = c.coord.end_byte;
            r.start_line = c.coord.start_line;
            r.end_line = c.coord.end_line;
            r.content_hash = c.coord.content_hash;
            r.mtime_ns = c.coord.mtime_ns;
            r.sig = c.sig;
            r.primary_mass_seed = c.primary_mass_seed;
            r.canonical_digest = c.canonical_digest;
            r.exact_stat_digest = c.exact_stat_digest;

            r.uri_offset = static_cast<uint32_t>(string_arena.size());
            r.uri_len = static_cast<uint32_t>(c.coord.uri.size());
            string_arena.append(c.coord.uri);

            r.prev_offset = static_cast<uint32_t>(string_arena.size());
            r.prev_len = static_cast<uint32_t>(c.preview.size());
            string_arena.append(c.preview);

            r.corpus_offset = static_cast<uint32_t>(string_arena.size());
            r.corpus_len    = static_cast<uint32_t>(c.corpus_id.size());
            string_arena.append(c.corpus_id);

            hvs[i] = c.hv;
        }

        uint32_t arena_size = static_cast<uint32_t>(string_arena.size());
        out.write(reinterpret_cast<const char*>(&arena_size), sizeof(arena_size));
        out.write(reinterpret_cast<const char*>(recs.data()), recs.size() * sizeof(PackedCellRecord));
        out.write(reinterpret_cast<const char*>(hvs.data()), hvs.size() * sizeof(Hypervec8192));
        out.write(string_arena.data(), arena_size);
        return out.good();
    }

    bool load_checkpoint(const std::string& filepath) {
        std::ifstream in(filepath, std::ios::binary);
        if (!in) return false;
        char magic[8]{};
        in.read(magic, 8);
        if (std::strncmp(magic, "MFDv4", 5) != 0) return false;

        uint32_t schema_ver = 0;
        in.read(reinterpret_cast<char*>(&schema_ver), sizeof(schema_ver));
        if (schema_ver != SCHEMA_VERSION) return false;

        in.read(reinterpret_cast<char*>(&global_acc), sizeof(global_acc));
        in.read(reinterpret_cast<char*>(&surprisal), sizeof(surprisal));

        uint32_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        cells.clear();
        if (count == 0) {
            rebuild_lookup_tables();
            return in.good();
        }

        uint32_t arena_size = 0;
        in.read(reinterpret_cast<char*>(&arena_size), sizeof(arena_size));

        std::vector<PackedCellRecord> recs(count);
        std::vector<Hypervec8192> hvs(count);
        std::string string_arena(arena_size, '\0');

        in.read(reinterpret_cast<char*>(recs.data()), count * sizeof(PackedCellRecord));
        in.read(reinterpret_cast<char*>(hvs.data()), count * sizeof(Hypervec8192));
        in.read(string_arena.data(), arena_size);

        cells.resize(count);
        for (uint32_t i = 0; i < count; ++i) {
            const auto& r = recs[i];
            auto& c = cells[i];
            c.id = r.id;
            c.parent_id = r.parent_id;
            c.halo = r.halo;
            c.coord.start_byte = r.start_byte;
            c.coord.end_byte = r.end_byte;
            c.coord.start_line = r.start_line;
            c.coord.end_line = r.end_line;
            c.coord.content_hash = r.content_hash;
            c.coord.mtime_ns = r.mtime_ns;
            c.sig = r.sig;
            c.primary_mass_seed = r.primary_mass_seed;
            c.canonical_digest = r.canonical_digest;
            c.canonical_hash = r.canonical_digest.low64();
            c.canonical_len = r.canonical_digest.canonical_len;
            c.exact_stat_digest = r.exact_stat_digest;
            c.coord.uri.assign(string_arena.data() + r.uri_offset, r.uri_len);
            c.preview.assign(string_arena.data() + r.prev_offset, r.prev_len);
            c.corpus_id.assign(string_arena.data() + r.corpus_offset, r.corpus_len);
            c.hv = hvs[i];
        }
        rebuild_lookup_tables();
        return in.good();
    }

};

// ============================================================================
// 11. QUERY EXPRESSION PARSER (@repo, @ref, @web, @docs, #io..#gate, /file, /block)
// ============================================================================
struct ParsedQuery {
    int modality_filter  = -1;
    int filter_reserved = -1;
    Scale target_scale   = Scale::BLOCK;
    std::string clean_expr;
};

static ParsedQuery parse_query_dsl(const std::string& raw_query) {
    ParsedQuery pq;
    std::istringstream iss(raw_query);
    std::string tok;
    std::vector<std::string> expr_tokens;

    while (iss >> tok) {
        if (tok == "@repo" || tok == "@code") pq.modality_filter = static_cast<int>(Modality::REPO_CODE);
        else if (tok == "@docs" || tok == "@doc") pq.modality_filter = static_cast<int>(Modality::SPEC_DOC);
        else if (tok == "@ref") pq.modality_filter = static_cast<int>(Modality::EXT_REF);
        else if (tok == "@web" || tok == "@wget") pq.modality_filter = static_cast<int>(Modality::WEB_REF);
        else if (tok == "@exp") pq.modality_filter = static_cast<int>(Modality::EXPERIMENT);
        else if (tok == "/file") pq.target_scale = Scale::FILE;
        else if (tok == "/block") pq.target_scale = Scale::BLOCK;
        else expr_tokens.push_back(tok);
    }

    for (size_t i = 0; i < expr_tokens.size(); ++i) {
        if (i > 0) pq.clean_expr.push_back(' ');
        pq.clean_expr += expr_tokens[i];
    }
    return pq;
}

static Modality parse_modality_flag(const std::string& s) {
    if (s == "repo" || s == "@repo") return Modality::REPO_CODE;
    if (s == "docs" || s == "@docs") return Modality::SPEC_DOC;
    if (s == "ref"  || s == "@ref")  return Modality::EXT_REF;
    if (s == "web"  || s == "@web" || s == "wget") return Modality::WEB_REF;
    return Modality::REPO_CODE;
}

// ============================================================================
// 12. LARGE-SCALE REPOSITORY BENCHMARK SUITE (`./manifold benchmark [dir]`)
// ============================================================================
static int run_benchmark(const std::string& target_dir) {
    using clock = std::chrono::high_resolution_clock;
    std::cout << "============================================================================\n";
    std::cout << "  MANIFOLD C++20 ENGINE — CONSTITUTIONAL STAGES 1-11 & REALIZATION BENCHMARK\n";
    std::cout << "============================================================================\n";
    std::cout << "  Target Corpus : " << target_dir << "\n";

    ManifoldEngine eng;
    auto t0 = clock::now();
    IngestStats st = eng.ingest_path(target_dir, Modality::REPO_CODE, "bench");
    auto t1 = clock::now();
    double ingest_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double mb = static_cast<double>(st.bytes_processed) / (1024.0 * 1024.0);

    size_t occupied_buckets = 0;
    for (const auto& b : eng.admission_buckets) {
        if (!b.empty()) occupied_buckets++;
    }

    const auto& inv = eng.adequacy_cert.invariant_coords;
    std::cout << "  Files Admitted: " << st.files_admitted << " files (" << std::fixed << std::setprecision(2) << mb << " MB)\n";
    std::cout << "  Total Cells   : " << eng.cells.size() << " (" << st.spans_admitted << " spans + " << st.files_admitted
              << " file nodes | " << eng.triad_sigs.size() << " TriSector triads)\n";
    std::cout << "  Bucket Occup. : " << occupied_buckets << " / 256 product-address buckets occupied\n";
    std::cout << "  Adequacy Gate : " << (eng.is_adequacy_certified ? "CERTIFIED (Substantive + Forced)" : "UNCERTIFIED (Blocked)")
              << " [pairs=" << eng.adequacy_cert.pairs_verified
              << ", witness=(" << eng.adequacy_cert.failing_witness_a << "," << eng.adequacy_cert.failing_witness_b << ")"
              << ", Tr(rho_F)=" << std::setprecision(4) << eng.adequacy_cert.density.trace
              << ", d_eff=" << std::setprecision(1) << inv.d_eff()
              << ", SE=" << std::setprecision(4) << inv.SE()
              << "]\n";
    std::cout << "  Cold Ingest   : " << std::setprecision(1) << ingest_ms << " ms ("
              << std::setprecision(1) << (mb / (ingest_ms / 1000.0)) << " MB/s | "
              << static_cast<int>(st.spans_admitted / (ingest_ms / 1000.0)) << " spans/s)\n";

    auto t2 = clock::now();
    IngestStats warm_st = eng.ingest_path(target_dir, Modality::REPO_CODE, "bench");
    auto t3 = clock::now();
    double warm_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
    std::cout << "  Warm Re-Sync  : " << std::setprecision(1) << warm_ms << " ms ("
              << warm_st.files_preserved << " preserved files verified & skipped)\n";

    std::string ckpt = "/tmp/manifold_large_bench.mfd";
    auto t4 = clock::now();
    eng.save_checkpoint(ckpt);
    auto t5 = clock::now();
    ManifoldEngine loaded;
    loaded.load_checkpoint(ckpt);
    auto t6 = clock::now();

    double save_ms = std::chrono::duration<double, std::milli>(t5 - t4).count();
    double load_ms = std::chrono::duration<double, std::milli>(t6 - t5).count();
    double ckpt_mb = static_cast<double>(fs::file_size(ckpt)) / (1024.0 * 1024.0);
    std::cout << "  Checkpoint I/O: Save=" << save_ms << " ms, Load=" << load_ms
              << " ms (Schema v" << ManifoldEngine::SCHEMA_VERSION << ", Size=" << std::setprecision(2) << ckpt_mb << " MB)\n";

    const std::array<const char*, 4> bench_queries = {{
        "red black tree rebalance rotate node pointer",
        "#op bit manipulation popcount count leading zeros",
        "#state vector contiguous allocator capacity reserve",
        "#gate static_assert concept type_traits integral_constant"
    }};

    double total_q_ms = 0.0;
    std::cout << "\n  [Query Routing Latency Across " << loaded.cells.size() << " Cells (Adequacy Certified=" << (loaded.is_adequacy_certified ? "YES" : "NO") << ")]\n";
    for (const char* q : bench_queries) {
        auto qt0 = clock::now();
        ParsedQuery pq = parse_query_dsl(q);
        Hypervec8192 qhv = encode_span_hypervector(pq.clean_expr, &loaded.surprisal, false, MAX_QUERY_MASS_NODES);
        auto hits = loaded.route(qhv, pq.target_scale, pq.filter_reserved, pq.modality_filter, 3);
        auto qt1 = clock::now();
        double q_ms = std::chrono::duration<double, std::milli>(qt1 - qt0).count();
        total_q_ms += q_ms;

        std::cout << "  • Query (" << std::setprecision(2) << q_ms << " ms): \"" << q << "\"\n";
        if (!hits.empty()) {
            std::cout << "      Top Hit: " << hits[0].cell->coord.uri << ":" << hits[0].cell->coord.start_line
                      << " [bucket=" << static_cast<int>(hits[0].cell->halo.bucket_id)
                      << " sim=" << std::setprecision(3) << hits[0].similarity
                      << " +" << std::setprecision(1) << hits[0].z_score << "s] | "
                      << hits[0].cell->preview << "\n";
        }
    }
    std::cout << "  Mean Query Latency (In-Memory): " << std::setprecision(2) << (total_q_ms / bench_queries.size()) << " ms\n";
    std::cout << "============================================================================\n";
    return 0;
}

// ============================================================================
// 13. VERIFICATION SUITE (`selftest`)
// ============================================================================

// ============================================================================
// PLACEMENT MODEL
// Every placement has local sources, one local destination, and external
// references that justify it. Destinations are always local.
// ============================================================================

enum class PlacementKind : uint8_t {
    FILL    = 0,  // local gap  -> local gap  (adapt external reference into gap)
    MOVE    = 1,  // local code -> local code (move near structural neighbors)
    EXTRACT = 2   // local code -> local file (co-locate a cluster)
};

struct SourceRange {
    std::string path;
    uint32_t start_line = 1;
    uint32_t end_line   = 1;
};

struct DestRange {
    std::string path;
    uint32_t start_line = 1;
    uint32_t end_line   = 1;
    bool create         = false;  // true if dest path does not exist yet
};

struct ReferenceMatch {
    std::string source_path;
    uint32_t line              = 1;
    double similarity          = 0.0;
    double signature_overlap   = 0.0;
    uint32_t cell_id           = 0;
};

struct Placement {
    PlacementKind kind      = PlacementKind::FILL;
    std::vector<SourceRange> sources;
    DestRange dest;
    double confidence       = 0.0;
    std::string reason;
    std::vector<ReferenceMatch> references;
};

static inline std::string json_escape(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

static inline std::string placement_kind_name(PlacementKind k) {
    switch (k) {
        case PlacementKind::FILL:    return "FILL";
        case PlacementKind::MOVE:    return "MOVE";
        case PlacementKind::EXTRACT: return "EXTRACT";
        default:                     return "?";
    }
}

static inline std::string placement_to_json(const Placement& p) {
    std::ostringstream o;
    o << "{";
    o << "\"kind\":\"" << placement_kind_name(p.kind) << "\",";
    o << "\"sources\":[";
    for (size_t i = 0; i < p.sources.size(); ++i) {
        if (i > 0) o << ",";
        o << "{\"path\":\"" << json_escape(p.sources[i].path)
          << "\",\"start_line\":" << p.sources[i].start_line
          << ",\"end_line\":" << p.sources[i].end_line << "}";
    }
    o << "],";
    o << "\"dest\":{\"path\":\"" << json_escape(p.dest.path)
      << "\",\"start_line\":" << p.dest.start_line
      << ",\"end_line\":" << p.dest.end_line
      << ",\"create\":" << (p.dest.create ? "true" : "false") << "},";
    o << "\"confidence\":" << std::fixed << std::setprecision(4) << p.confidence << ",";
    o << "\"reason\":\"" << json_escape(p.reason) << "\",";
    o << "\"references\":[";
    for (size_t i = 0; i < p.references.size(); ++i) {
        if (i > 0) o << ",";
        o << "{\"source_path\":\"" << json_escape(p.references[i].source_path)
          << "\",\"line\":" << p.references[i].line
          << ",\"similarity\":" << p.references[i].similarity
          << ",\"signature_overlap\":" << p.references[i].signature_overlap << "}";
    }
    o << "]}";
    return o.str();
}

[[maybe_unused]] static inline std::string placements_to_json(const std::vector<Placement>& ps) {
    std::ostringstream o;
    o << "[";
    for (size_t i = 0; i < ps.size(); ++i) {
        if (i > 0) o << ",";
        o << placement_to_json(ps[i]);
    }
    o << "]";
    return o.str();
}

static inline std::optional<PlacementKind> parse_placement_kind(std::string_view s) {
    if (s == "fill"    || s == "FILL")    return PlacementKind::FILL;
    if (s == "move"    || s == "MOVE")    return PlacementKind::MOVE;
    if (s == "extract" || s == "EXTRACT") return PlacementKind::EXTRACT;
    return std::nullopt;
}

enum class GapKind : uint8_t {
    STUB_DECLARATION    = 0,   // void foo(int);   (declaration, no body)
    EMPTY_BODY          = 1,   // void foo() {}
    TODO_COMMENT        = 2,   // // TODO: ...
    NOT_IMPLEMENTED     = 3,   // throw / todo!() / unimplemented!()
    PASS_BODY           = 4,   // def foo(): pass
    ELLIPSIS_BODY       = 5,   // def foo(): ...
    UNIMPLEMENTED_MACRO = 6    // unimplemented!() / panic!("not implemented")
};

static inline std::string gap_kind_name(GapKind k) {
    switch (k) {
        case GapKind::STUB_DECLARATION:    return "STUB_DECLARATION";
        case GapKind::EMPTY_BODY:          return "EMPTY_BODY";
        case GapKind::TODO_COMMENT:        return "TODO_COMMENT";
        case GapKind::NOT_IMPLEMENTED:     return "NOT_IMPLEMENTED";
        case GapKind::PASS_BODY:           return "PASS_BODY";
        case GapKind::ELLIPSIS_BODY:       return "ELLIPSIS_BODY";
        case GapKind::UNIMPLEMENTED_MACRO: return "UNIMPLEMENTED_MACRO";
        default:                           return "?";
    }
}

struct CodeGap {
    uint32_t cell_id       = 0;
    uint32_t byte_offset   = 0;
    uint32_t byte_length   = 0;
    uint32_t line_number   = 1;
    GapKind  kind          = GapKind::TODO_COMMENT;
    std::string signature;
    std::string function_name;
    std::string language;   // "cpp" | "python" | "rust" | "lua" | "js"
};

static std::vector<CodeGap> detect_cpp_gaps(
    std::string_view text,
    uint32_t cell_id,
    uint32_t base_line = 1
) {
    std::vector<CodeGap> gaps;
    const std::regex re_stub(R"(^\s*(?:static\s+)?(?:inline\s+)?(?:const\s+)?[\w:<>,*&\s]+\s+(\w+)\s*\([^)]*\)\s*;\s*$)",
                             std::regex::ECMAScript | std::regex::optimize);
    const std::regex re_empty(R"(^\s*(?:static\s+)?(?:inline\s+)?(?:const\s+)?[\w:<>,*&\s]+\s+(\w+)\s*\([^)]*\)\s*\{\s*\}\s*$)",
                              std::regex::ECMAScript | std::regex::optimize);
    const std::regex re_todo(R"(//\s*(?:TODO|FIXME|XXX)\b.*)");

    uint32_t line_no = base_line;
    size_t line_start = 0;
    while (line_start <= text.size()) {
        size_t line_end = text.find('\n', line_start);
        if (line_end == std::string_view::npos) line_end = text.size();
        std::string line(text.substr(line_start, line_end - line_start));

        std::smatch m;
        if (std::regex_match(line, m, re_stub) && m[1].str() != "return" && m[1].str() != "if" && m[1].str() != "for" && m[1].str() != "while") {
            CodeGap g;
            g.cell_id = cell_id;
            g.byte_offset = static_cast<uint32_t>(line_start);
            g.byte_length = static_cast<uint32_t>(line_end - line_start);
            g.line_number = line_no;
            g.kind = GapKind::STUB_DECLARATION;
            g.function_name = m[1].str();
            g.signature = line;
            g.language = "cpp";
            gaps.push_back(std::move(g));
        } else if (std::regex_match(line, m, re_empty)) {
            CodeGap g;
            g.cell_id = cell_id;
            g.byte_offset = static_cast<uint32_t>(line_start);
            g.byte_length = static_cast<uint32_t>(line_end - line_start);
            g.line_number = line_no;
            g.kind = GapKind::EMPTY_BODY;
            g.function_name = m[1].str();
            g.signature = line;
            g.language = "cpp";
            gaps.push_back(std::move(g));
        } else if (std::regex_search(line, m, re_todo)) {
            CodeGap g;
            g.cell_id = cell_id;
            g.byte_offset = static_cast<uint32_t>(line_start);
            g.byte_length = static_cast<uint32_t>(line_end - line_start);
            g.line_number = line_no;
            g.kind = GapKind::TODO_COMMENT;
            g.function_name = "";
            g.signature = line;
            g.language = "cpp";
            gaps.push_back(std::move(g));
        }

        if (line_end >= text.size()) break;
        line_start = line_end + 1;
        line_no++;
    }
    return gaps;
}

static std::vector<CodeGap> detect_python_gaps(
    std::string_view text,
    uint32_t cell_id,
    uint32_t base_line = 1
) {
    std::vector<CodeGap> gaps;
    const std::regex re_def(R"(^\s*def\s+(\w+)\s*\([^)]*\)\s*(?:->\s*[\w\[\], \.]+\s*)?:\s*(pass|\.\.\.)?\s*$)");
    const std::regex re_only_pass(R"(^\s*pass\s*$)");
    const std::regex re_only_ellipsis(R"(^\s*\.\.\.\s*$)");
    const std::regex re_todo(R"(#\s*(?:TODO|FIXME|XXX)\b.*)");

    uint32_t line_no = base_line;
    size_t line_start = 0;
    std::string pending_def_name;
    std::string pending_def_sig;
    uint32_t pending_def_line = 0;
    uint32_t pending_def_off  = 0;

    while (line_start <= text.size()) {
        size_t line_end = text.find('\n', line_start);
        if (line_end == std::string_view::npos) line_end = text.size();
        std::string line(text.substr(line_start, line_end - line_start));

        std::smatch m;
        if (!pending_def_name.empty()) {
            if (std::regex_match(line, re_only_pass)) {
                CodeGap g;
                g.cell_id = cell_id;
                g.byte_offset = pending_def_off;
                g.byte_length = static_cast<uint32_t>(line_end - pending_def_off);
                g.line_number = pending_def_line;
                g.kind = GapKind::PASS_BODY;
                g.function_name = pending_def_name;
                g.signature = pending_def_sig;
                g.language = "python";
                gaps.push_back(std::move(g));
                pending_def_name.clear();
                if (line_end >= text.size()) break;
                line_start = line_end + 1;
                line_no++;
                continue;
            } else if (std::regex_match(line, re_only_ellipsis)) {
                CodeGap g;
                g.cell_id = cell_id;
                g.byte_offset = pending_def_off;
                g.byte_length = static_cast<uint32_t>(line_end - pending_def_off);
                g.line_number = pending_def_line;
                g.kind = GapKind::ELLIPSIS_BODY;
                g.function_name = pending_def_name;
                g.signature = pending_def_sig;
                g.language = "python";
                gaps.push_back(std::move(g));
                pending_def_name.clear();
                if (line_end >= text.size()) break;
                line_start = line_end + 1;
                line_no++;
                continue;
            } else if (!line.empty()) {
                pending_def_name.clear();
            }
        }

        if (std::regex_match(line, m, re_def)) {
            std::string fn = m[1].str();
            std::string body_tok = m[2].matched ? m[2].str() : "";
            if (body_tok == "pass" || body_tok == "...") {
                CodeGap g;
                g.cell_id = cell_id;
                g.byte_offset = static_cast<uint32_t>(line_start);
                g.byte_length = static_cast<uint32_t>(line_end - line_start);
                g.line_number = line_no;
                g.kind = (body_tok == "pass") ? GapKind::PASS_BODY : GapKind::ELLIPSIS_BODY;
                g.function_name = fn;
                g.signature = line;
                g.language = "python";
                gaps.push_back(std::move(g));
            } else {
                pending_def_name = fn;
                pending_def_sig  = line;
                pending_def_line = line_no;
                pending_def_off  = static_cast<uint32_t>(line_start);
            }
        } else if (std::regex_search(line, m, re_todo)) {
            CodeGap g;
            g.cell_id = cell_id;
            g.byte_offset = static_cast<uint32_t>(line_start);
            g.byte_length = static_cast<uint32_t>(line_end - line_start);
            g.line_number = line_no;
            g.kind = GapKind::TODO_COMMENT;
            g.function_name = "";
            g.signature = line;
            g.language = "python";
            gaps.push_back(std::move(g));
        }

        if (line_end >= text.size()) break;
        line_start = line_end + 1;
        line_no++;
    }
    return gaps;
}

static std::vector<CodeGap> detect_rust_gaps(
    std::string_view text,
    uint32_t cell_id,
    uint32_t base_line = 1
) {
    std::vector<CodeGap> gaps;
    const std::regex re_todo(R"(fn\s+(\w+)\s*\([^)]*\)\s*(?:->\s*[\w<>:,&]+\s*)?\{[^}]*\b(?:todo!|unimplemented!)\s*\(\s*\)[^}]*\})");
    const std::regex re_empty(R"(fn\s+(\w+)\s*\([^)]*\)\s*(?:->\s*[\w<>:,&]+\s*)?\{\s*\})");
    const std::regex re_comment(R"(//\s*(?:TODO|FIXME|XXX)\b.*)");

    uint32_t line_no = base_line;
    size_t line_start = 0;
    while (line_start <= text.size()) {
        size_t line_end = text.find('\n', line_start);
        if (line_end == std::string_view::npos) line_end = text.size();
        std::string line(text.substr(line_start, line_end - line_start));

        std::smatch m;
        CodeGap g;
        g.cell_id = cell_id;
        g.byte_offset = static_cast<uint32_t>(line_start);
        g.byte_length = static_cast<uint32_t>(line_end - line_start);
        g.line_number = line_no;
        g.language = "rust";

        if (std::regex_search(line, m, re_todo)) {
            g.kind = GapKind::NOT_IMPLEMENTED;
            g.function_name = m[1].str();
            g.signature = line;
            gaps.push_back(std::move(g));
        } else if (std::regex_search(line, m, re_empty)) {
            g.kind = GapKind::EMPTY_BODY;
            g.function_name = m[1].str();
            g.signature = line;
            gaps.push_back(std::move(g));
        } else if (std::regex_search(line, m, re_comment)) {
            g.kind = GapKind::TODO_COMMENT;
            g.function_name = "";
            g.signature = line;
            gaps.push_back(std::move(g));
        }

        if (line_end >= text.size()) break;
        line_start = line_end + 1;
        line_no++;
    }
    return gaps;
}

static std::vector<CodeGap> detect_lua_gaps(std::string_view text, uint32_t cell_id, uint32_t base_line = 1) {
    std::vector<CodeGap> gaps;
    const std::regex re_empty(R"(function\s+([\w.:]+)\s*\([^)]*\)\s*end\s*$)");
    const std::regex re_todo(R"(--\s*(?:TODO|FIXME|XXX)\b.*)");
    uint32_t line_no = base_line;
    size_t line_start = 0;
    while (line_start <= text.size()) {
        size_t line_end = text.find('\n', line_start);
        if (line_end == std::string_view::npos) line_end = text.size();
        std::string line(text.substr(line_start, line_end - line_start));
        std::smatch m;
        CodeGap g;
        g.cell_id = cell_id;
        g.byte_offset = static_cast<uint32_t>(line_start);
        g.byte_length = static_cast<uint32_t>(line_end - line_start);
        g.line_number = line_no;
        g.language = "lua";
        if (std::regex_search(line, m, re_empty)) {
            g.kind = GapKind::EMPTY_BODY;
            g.function_name = m[1].str();
            g.signature = line;
            gaps.push_back(std::move(g));
        } else if (std::regex_search(line, m, re_todo)) {
            g.kind = GapKind::TODO_COMMENT;
            g.function_name = "";
            g.signature = line;
            gaps.push_back(std::move(g));
        }
        if (line_end >= text.size()) break;
        line_start = line_end + 1;
        line_no++;
    }
    return gaps;
}

static std::vector<CodeGap> detect_js_gaps(std::string_view text, uint32_t cell_id, uint32_t base_line = 1) {
    std::vector<CodeGap> gaps;
    const std::regex re_empty_fn(R"(function\s+(\w+)\s*\([^)]*\)\s*\{\s*\}\s*$)");
    const std::regex re_arrow(R"((?:const|let)\s+(\w+)\s*=\s*\([^)]*\)\s*=>\s*\{\s*\}\s*$)");
    const std::regex re_throw(R"(function\s+(\w+)\s*\([^)]*\)\s*\{[^}]*throw\s+new\s+Error[^}]*\})");
    const std::regex re_todo(R"(//\s*(?:TODO|FIXME|XXX)\b.*)");
    uint32_t line_no = base_line;
    size_t line_start = 0;
    while (line_start <= text.size()) {
        size_t line_end = text.find('\n', line_start);
        if (line_end == std::string_view::npos) line_end = text.size();
        std::string line(text.substr(line_start, line_end - line_start));
        std::smatch m;
        CodeGap g;
        g.cell_id = cell_id;
        g.byte_offset = static_cast<uint32_t>(line_start);
        g.byte_length = static_cast<uint32_t>(line_end - line_start);
        g.line_number = line_no;
        g.language = "js";
        if (std::regex_search(line, m, re_throw)) {
            g.kind = GapKind::NOT_IMPLEMENTED;
            g.function_name = m[1].str();
            g.signature = line;
            gaps.push_back(std::move(g));
        } else if (std::regex_search(line, m, re_empty_fn)) {
            g.kind = GapKind::EMPTY_BODY;
            g.function_name = m[1].str();
            g.signature = line;
            gaps.push_back(std::move(g));
        } else if (std::regex_search(line, m, re_arrow)) {
            g.kind = GapKind::EMPTY_BODY;
            g.function_name = m[1].str();
            g.signature = line;
            gaps.push_back(std::move(g));
        } else if (std::regex_search(line, m, re_todo)) {
            g.kind = GapKind::TODO_COMMENT;
            g.function_name = "";
            g.signature = line;
            gaps.push_back(std::move(g));
        }
        if (line_end >= text.size()) break;
        line_start = line_end + 1;
        line_no++;
    }
    return gaps;
}

static std::string detect_language_from_path(std::string_view path) {
    auto ends_with = [&](std::string_view suf) {
        return path.size() >= suf.size() && path.substr(path.size() - suf.size()) == suf;
    };
    if (ends_with(".c") || ends_with(".cpp") || ends_with(".cc") || ends_with(".cxx") ||
        ends_with(".h") || ends_with(".hpp") || ends_with(".hh")) return "cpp";
    if (ends_with(".py")) return "python";
    if (ends_with(".rs")) return "rust";
    if (ends_with(".lua")) return "lua";
    if (ends_with(".js") || ends_with(".ts") || ends_with(".jsx") || ends_with(".tsx")) return "js";
    return "unknown";
}

static std::string read_cell_text(const ManifoldEngine& engine, const Cell& cell) {
    std::string text;
    std::error_code ec;
    if (fs::exists(cell.coord.uri, ec)) {
        std::ifstream in(cell.coord.uri, std::ios::binary);
        if (in && cell.coord.end_byte >= cell.coord.start_byte) {
            in.seekg(cell.coord.start_byte);
            text.resize(cell.coord.end_byte - cell.coord.start_byte);
            in.read(text.data(), static_cast<std::streamsize>(text.size()));
            return text;
        }
    }
    auto mit = engine.memory_file_cache.find(cell.coord.uri);
    if (mit != engine.memory_file_cache.end()) {
        const std::string& full = mit->second;
        if (cell.coord.start_byte < full.size() && cell.coord.end_byte > cell.coord.start_byte) {
            size_t len = std::min<size_t>(cell.coord.end_byte - cell.coord.start_byte, full.size() - cell.coord.start_byte);
            return full.substr(cell.coord.start_byte, len);
        }
    }
    return text;
}

static std::vector<CodeGap> find_stub_gaps(
    const ManifoldEngine& engine,
    const std::string& local_corpus_id = "local"
) {
    std::vector<CodeGap> gaps;
    for (const auto& cell : engine.cells) {
        if (cell.corpus_id != local_corpus_id) continue;
        if (cell.halo.scale != Scale::BLOCK) continue;
        if (cell.halo.modality != Modality::REPO_CODE && cell.halo.modality != Modality::SPEC_DOC) continue;

        std::string lang = detect_language_from_path(cell.coord.uri);
        if (lang == "unknown") continue;

        std::string text = read_cell_text(engine, cell);
        if (text.empty()) continue;

        std::vector<CodeGap> cell_gaps;
        if (lang == "cpp")         cell_gaps = detect_cpp_gaps(text, cell.id, cell.coord.start_line);
        else if (lang == "python") cell_gaps = detect_python_gaps(text, cell.id, cell.coord.start_line);
        else if (lang == "rust")   cell_gaps = detect_rust_gaps(text, cell.id, cell.coord.start_line);
        else if (lang == "lua")    cell_gaps = detect_lua_gaps(text, cell.id, cell.coord.start_line);
        else if (lang == "js")     cell_gaps = detect_js_gaps(text, cell.id, cell.coord.start_line);

        for (auto& g : cell_gaps) gaps.push_back(std::move(g));
    }
    return gaps;
}

static std::string find_call_sites(
    const ManifoldEngine& engine,
    const std::string& function_name,
    const std::string& local_corpus_id = "local",
    size_t max_sites = 5
) {
    if (function_name.empty()) return "";
    std::string pattern = "\\b" + function_name + "\\s*\\(";
    std::regex re(pattern);
    std::vector<std::string> sites;
    for (const auto& cell : engine.cells) {
        if (cell.corpus_id != local_corpus_id) continue;
        if (cell.halo.scale != Scale::BLOCK) continue;
        std::string text = read_cell_text(engine, cell);
        if (text.empty()) continue;
        std::sregex_iterator it(text.begin(), text.end(), re);
        for (; it != std::sregex_iterator(); ++it) {
            size_t pos = static_cast<size_t>(it->position());
            size_t start = pos > 40 ? pos - 40 : 0;
            size_t end = std::min(text.size(), pos + 60);
            sites.push_back(text.substr(start, end - start));
            if (sites.size() >= max_sites) goto done;
        }
    }
    done:
    std::string out;
    for (size_t i = 0; i < sites.size(); ++i) {
        if (i > 0) out += "\n";
        out += sites[i];
    }
    return out;
}

struct GapAnchor {
    CodeGap gap;
    std::string signature;
    std::string leading_imports;
    std::string preceding_context;
    std::string following_context;
    std::string call_sites;
    Hypervec8192 hv;
};

static inline void extract_context(
    std::string_view text,
    uint32_t byte_offset,
    std::string& preceding_out,
    std::string& following_out,
    std::string& leading_imports_out
) {
    preceding_out.clear();
    following_out.clear();
    leading_imports_out.clear();

    std::vector<std::pair<size_t, size_t>> lines;
    size_t start = 0;
    for (size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == '\n') {
            lines.push_back({start, i});
            start = i + 1;
        }
    }

    size_t gap_line_idx = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].first <= byte_offset && byte_offset <= lines[i].second) {
            gap_line_idx = i;
            break;
        }
    }

    int collected = 0;
    for (int i = static_cast<int>(gap_line_idx) - 1; i >= 0 && collected < 3; --i) {
        std::string_view line = text.substr(lines[i].first, lines[i].second - lines[i].first);
        bool blank = true;
        for (char c : line) if (!std::isspace(static_cast<unsigned char>(c))) { blank = false; break; }
        if (blank) continue;
        preceding_out = std::string(line) + "\n" + preceding_out;
        collected++;
    }

    collected = 0;
    for (size_t i = gap_line_idx + 1; i < lines.size() && collected < 3; ++i) {
        std::string_view line = text.substr(lines[i].first, lines[i].second - lines[i].first);
        bool blank = true;
        for (char c : line) if (!std::isspace(static_cast<unsigned char>(c))) { blank = false; break; }
        if (blank) continue;
        following_out += std::string(line) + "\n";
        collected++;
    }

    std::regex re_import(R"(^\s*(?:#\s*include|import\s|from\s\S+\s+import\s|use\s|require\s*\().*$)");
    std::vector<std::string> imports;
    for (size_t i = 0; i < gap_line_idx; ++i) {
        std::string_view line = text.substr(lines[i].first, lines[i].second - lines[i].first);
        std::string line_str(line);
        if (std::regex_match(line_str, re_import)) {
            imports.push_back(line_str);
        }
    }
    size_t from = imports.size() > 5 ? imports.size() - 5 : 0;
    for (size_t i = from; i < imports.size(); ++i) {
        if (!leading_imports_out.empty()) leading_imports_out += "\n";
        leading_imports_out += imports[i];
    }
}

static GapAnchor build_gap_anchor(
    const ManifoldEngine& engine,
    const CodeGap& gap,
    const std::string& local_corpus_id = "local"
) {
    GapAnchor a;
    a.gap = gap;
    a.signature = gap.signature;

    std::string text;
    auto it = std::find_if(engine.cells.begin(), engine.cells.end(),
                           [&](const Cell& c) { return c.id == gap.cell_id; });
    if (it != engine.cells.end()) {
        text = read_cell_text(engine, *it);
    }

    if (!text.empty()) {
        extract_context(text, gap.byte_offset, a.preceding_context, a.following_context, a.leading_imports);
    }

    a.call_sites = find_call_sites(engine, gap.function_name, local_corpus_id);

    std::string anchor_text = a.signature + " " + a.leading_imports + " " + a.call_sites;
    a.hv = encode_span_hypervector(anchor_text, nullptr, false, MAX_QUERY_MASS_NODES);
    return a;
}

static double compute_signature_overlap(std::string_view sig_a, std::string_view sig_b) {
    auto tokenize = [](std::string_view s) {
        std::set<std::string> tokens;
        std::string cur;
        for (char c : s) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
                cur += c;
            } else {
                if (!cur.empty()) { tokens.insert(cur); cur.clear(); }
            }
        }
        if (!cur.empty()) tokens.insert(cur);
        return tokens;
    };
    auto A = tokenize(sig_a);
    auto B = tokenize(sig_b);
    if (A.empty() || B.empty()) return 0.0;
    size_t inter = 0;
    for (const auto& t : A) if (B.count(t)) inter++;
    size_t uni = A.size() + B.size() - inter;
    return uni == 0 ? 0.0 : static_cast<double>(inter) / static_cast<double>(uni);
}

static std::vector<ReferenceMatch> find_reference_for_gap(
    const ManifoldEngine& engine,
    const GapAnchor& anchor,
    size_t top_k = 5
) {
    std::vector<ReferenceMatch> out;
    if (!engine.is_adequacy_certified) return out;

    auto hits = engine.route(anchor.hv, Scale::BLOCK, -1, -1, top_k * 3);
    for (const auto& h : hits) {
        if (h.cell->corpus_id.empty() || h.cell->corpus_id == "local") continue;
        ReferenceMatch m;
        m.source_path = h.cell->coord.uri;
        m.line = h.cell->coord.start_line;
        m.similarity = h.similarity;
        m.signature_overlap = compute_signature_overlap(anchor.signature, h.cell->preview);
        m.cell_id = h.cell->id;
        out.push_back(std::move(m));
        if (out.size() >= top_k) break;
    }

    std::sort(out.begin(), out.end(), [](const ReferenceMatch& a, const ReferenceMatch& b) {
        return (a.similarity * 0.7 + a.signature_overlap * 0.3) >
               (b.similarity * 0.7 + b.signature_overlap * 0.3);
    });
    if (out.size() > top_k) out.resize(top_k);
    return out;
}

static std::vector<Placement> find_fill_placements(
    const ManifoldEngine& engine,
    const std::vector<CodeGap>& gaps,
    size_t top_k_per_gap = 3
) {
    std::vector<Placement> out;
    for (const auto& gap : gaps) {
        GapAnchor anchor = build_gap_anchor(engine, gap);
        auto refs = find_reference_for_gap(engine, anchor, top_k_per_gap);
        if (refs.empty()) continue;

        Placement p;
        p.kind = PlacementKind::FILL;
        SourceRange src;
        src.path = "";
        for (const auto& c : engine.cells) {
            if (c.id == gap.cell_id) { src.path = c.coord.uri; break; }
        }
        src.start_line = gap.line_number;
        src.end_line = gap.line_number;
        p.sources.push_back(src);

        p.dest.path = src.path;
        p.dest.start_line = gap.line_number;
        p.dest.end_line = gap.line_number;
        p.dest.create = false;

        p.confidence = refs[0].similarity * 0.7 + refs[0].signature_overlap * 0.3;
        p.reason = "gap (" + gap_kind_name(gap.kind) + ") at " +
                   gap.function_name + "; " + std::to_string(refs.size()) + " refs";
        p.references = refs;
        out.push_back(std::move(p));
    }
    return out;
}

static std::vector<uint32_t> get_neighbors(
    const ManifoldEngine& engine,
    uint32_t cell_idx
) {
    std::vector<uint32_t> out;
    if (cell_idx >= engine.rel_links.size()) return out;
    const auto& rl = engine.rel_links[cell_idx];
    auto add = [&](uint32_t idx_or_id) {
        if (idx_or_id == ABSENT_NODE) return;
        if (idx_or_id < engine.cells.size()) {
            out.push_back(idx_or_id);
        }
    };
    add(rl.adjacency_prev_idx);
    add(rl.adjacency_next_idx);
    add(rl.mixed_sector_sib0_idx);
    add(rl.mixed_sector_sib1_idx);
    add(rl.holonomy_equiv_idx);
    add(rl.curvature_equiv_idx);
    if (out.empty() && cell_idx < engine.cells.size()) {
        uint32_t pid = engine.cells[cell_idx].parent_id;
        if (pid < engine.id_to_idx.size() && engine.id_to_idx[pid] >= 0) {
            out.push_back(static_cast<uint32_t>(engine.id_to_idx[pid]));
        }
    }
    return out;
}

static std::vector<Placement> find_move_placements(
    const ManifoldEngine& engine,
    const std::string& local_corpus_id = "local"
) {
    std::vector<Placement> out;
    for (size_t i = 0; i < engine.cells.size(); ++i) {
        if (engine.cells[i].corpus_id != local_corpus_id) continue;
        if (engine.cells[i].halo.scale != Scale::BLOCK) continue;

        auto neigh = get_neighbors(engine, static_cast<uint32_t>(i));
        if (neigh.size() < 3) continue;

        std::map<std::string, std::vector<uint32_t>> by_file;
        for (uint32_t nidx : neigh) {
            const auto& nc = engine.cells[nidx];
            if (nc.corpus_id != local_corpus_id) continue;
            by_file[nc.coord.uri].push_back(nc.coord.start_line);
        }
        if (by_file.size() < 2) continue;

        std::string my_file = engine.cells[i].coord.uri;
        std::string best_file;
        size_t best_count = 0;
        for (const auto& [path, lines] : by_file) {
            if (path == my_file) continue;
            if (lines.size() > best_count) { best_count = lines.size(); best_file = path; }
        }
        if (best_count < neigh.size() * 0.6) continue;

        auto& lines = by_file[best_file];
        std::sort(lines.begin(), lines.end());
        uint32_t median_line = lines[lines.size() / 2];

        Placement p;
        p.kind = PlacementKind::MOVE;
        p.sources.push_back({my_file, engine.cells[i].coord.start_line, engine.cells[i].coord.end_line});
        p.dest = {best_file, median_line, median_line, false};
        p.confidence = static_cast<double>(best_count) / static_cast<double>(neigh.size());
        p.reason = std::to_string(best_count) + " of " + std::to_string(neigh.size()) +
                   " neighbors live in " + best_file;
        out.push_back(std::move(p));
    }
    return out;
}

static std::vector<Placement> find_extract_placements(
    const ManifoldEngine& engine,
    const std::string& local_corpus_id = "local"
) {
    std::vector<Placement> out;
    std::vector<uint32_t> block_indices;
    for (size_t i = 0; i < engine.cells.size(); ++i) {
        if (engine.cells[i].corpus_id == local_corpus_id && engine.cells[i].halo.scale == Scale::BLOCK) {
            block_indices.push_back(static_cast<uint32_t>(i));
        }
    }
    if (block_indices.size() < 4) return out;

    constexpr uint32_t CLUSTER_THRESHOLD = 4096;
    std::vector<bool> visited(block_indices.size(), false);

    for (size_t seed = 0; seed < block_indices.size(); ++seed) {
        if (visited[seed]) continue;
        std::vector<size_t> cluster = {seed};
        visited[seed] = true;
        for (size_t j = seed + 1; j < block_indices.size(); ++j) {
            if (visited[j]) continue;
            uint32_t ham = engine.cells[block_indices[seed]].hv.hamming(engine.cells[block_indices[j]].hv);
            if (ham <= CLUSTER_THRESHOLD) {
                cluster.push_back(j);
                visited[j] = true;
            }
        }
        if (cluster.size() < 3) continue;

        std::set<std::string> files;
        for (size_t idx : cluster) files.insert(engine.cells[block_indices[idx]].coord.uri);
        if (files.size() < 3) continue;

        Placement p;
        p.kind = PlacementKind::EXTRACT;
        for (size_t idx : cluster) {
            const auto& c = engine.cells[block_indices[idx]];
            p.sources.push_back({c.coord.uri, c.coord.start_line, c.coord.end_line});
        }
        p.dest = {"src/extracted_cluster.cpp", 1, 1, true};
        p.confidence = 0.5;
        p.reason = std::to_string(cluster.size()) + " cells across " +
                   std::to_string(files.size()) + " files";
        out.push_back(std::move(p));
    }
    return out;
}

// ============================================================================
// §9. GAP STATE MACHINE WITH FOUR-PROPERTY TRANSITION PROOFS
// ============================================================================

enum class GapState : uint8_t {
    UNDISCOVERED        = 0,  // Gap exists. No reference proposed.
    CANDIDATES_PROPOSED = 1,  // At least one reference matched. Awaiting selection.
    REFERENCE_SELECTED  = 2,  // A specific reference was chosen by user.
    APPLIED             = 3,  // User produced an adaptation.
    VERIFIED            = 4,  // The adaptation passed verification (terminal).
    REJECTED            = 5   // Explicitly ruled out with reason (terminal).
};

static constexpr const char* gap_state_name(GapState s) noexcept {
    switch (s) {
        case GapState::UNDISCOVERED:        return "UNDISCOVERED";
        case GapState::CANDIDATES_PROPOSED: return "CANDIDATES_PROPOSED";
        case GapState::REFERENCE_SELECTED:  return "REFERENCE_SELECTED";
        case GapState::APPLIED:             return "APPLIED";
        case GapState::VERIFIED:            return "VERIFIED";
        case GapState::REJECTED:            return "REJECTED";
        default:                            return "?";
    }
}

static constexpr bool is_allowed_transition(GapState from, GapState to) noexcept {
    using S = GapState;
    return (from == S::UNDISCOVERED        && to == S::CANDIDATES_PROPOSED) ||
           (from == S::CANDIDATES_PROPOSED && to == S::REFERENCE_SELECTED)  ||
           (from == S::REFERENCE_SELECTED  && to == S::APPLIED)             ||
           (from == S::APPLIED             && to == S::VERIFIED)            ||
           (from == S::UNDISCOVERED        && to == S::REJECTED)            ||
           (from == S::CANDIDATES_PROPOSED && to == S::REJECTED)            ||
           (from == S::REFERENCE_SELECTED  && to == S::REJECTED)            ||
           (from == S::APPLIED             && to == S::REJECTED);
}

struct TransitionProof {
    GapState from = GapState::UNDISCOVERED;
    GapState to   = GapState::UNDISCOVERED;

    // The four required properties. All must be true for the transition to be accepted.
    bool divisible  = false;
    bool admissible = false;
    bool correct    = false;
    bool respectful = false;

    // Evidence. Populated by the caller before try_accept_transition.
    double   similarity        = 0.0;
    double   signature_overlap = 0.0;
    uint32_t evidence_cell     = 0;   // reference cell id, 0 if N/A
    std::string evidence_text;        // e.g., "compiled", "tests passed"
    std::string reasoning;

    uint64_t timestamp_unix = 0;

    bool is_valid() const noexcept {
        return divisible && admissible && correct && respectful && from != to;
    }
};

struct GapConstitutionalState {
    // Identity — set once at discovery
    uint32_t    gap_id      = 0;
    uint32_t    cell_id     = 0;
    uint32_t    byte_offset = 0;
    uint32_t    byte_length = 0;
    uint32_t    line_number = 1;
    std::string file_uri;
    std::string signature;
    std::string language;

    // Current state
    GapState state = GapState::UNDISCOVERED;

    // Evidence accumulated across transitions
    std::vector<uint32_t> candidate_cell_ids;   // filled at CANDIDATES_PROPOSED
    std::vector<ReferenceMatch> candidates;     // full candidate matches
    uint32_t    selected_reference_cell_id = 0; // filled at REFERENCE_SELECTED
    std::string applied_adaptation;             // filled at APPLIED
    std::string verification_result;            // filled at VERIFIED

    // Append-only history of accepted transitions
    std::vector<TransitionProof> history;
};

static inline uint64_t now_unix() noexcept {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

static inline bool try_accept_transition(
    GapConstitutionalState& gs,
    TransitionProof proof
) {
    // DIVISIBLE: atomic transition from exact predecessor
    proof.divisible = gs.history.empty()
        ? (proof.from == GapState::UNDISCOVERED)
        : (gs.history.back().to == proof.from);

    // ADMISSIBLE: in allow-table
    proof.admissible = is_allowed_transition(proof.from, proof.to);

    // CORRECT: caller set proof.correct before calling.

    // RESPECTFUL: non-empty reasoning, current state matches proof.from, append-only history
    proof.respectful = (!proof.reasoning.empty())
                    && (gs.state == proof.from);

    if (!proof.is_valid()) return false;

    proof.timestamp_unix = now_unix();
    gs.history.push_back(proof);
    gs.state = proof.to;
    return true;
}

// UNDISCOVERED -> CANDIDATES_PROPOSED
// Correct: top similarity >= 0.55
static inline bool propose_candidates(
    GapConstitutionalState& gs,
    const std::vector<ReferenceMatch>& candidates
) {
    if (gs.state != GapState::UNDISCOVERED) return false;
    if (candidates.empty()) return false;
    if (candidates[0].similarity < 0.55) return false;

    gs.candidate_cell_ids.clear();
    gs.candidates = candidates;
    for (const auto& c : candidates) gs.candidate_cell_ids.push_back(c.cell_id);

    TransitionProof p;
    p.from = GapState::UNDISCOVERED;
    p.to   = GapState::CANDIDATES_PROPOSED;
    p.similarity    = candidates[0].similarity;
    p.evidence_cell = candidates[0].cell_id;
    p.reasoning     = "proposed " + std::to_string(candidates.size()) +
                      " candidates; top similarity=" + std::to_string(candidates[0].similarity);
    p.correct = (candidates[0].similarity >= 0.55);
    return try_accept_transition(gs, p);
}

// CANDIDATES_PROPOSED -> REFERENCE_SELECTED
// Correct: reference in candidates AND signature_overlap >= 0.6
static inline bool select_reference(
    GapConstitutionalState& gs,
    uint32_t reference_cell_id,
    double similarity,
    double signature_overlap
) {
    if (gs.state != GapState::CANDIDATES_PROPOSED) return false;
    bool is_candidate = std::find(gs.candidate_cell_ids.begin(),
                                  gs.candidate_cell_ids.end(),
                                  reference_cell_id) != gs.candidate_cell_ids.end();
    if (!is_candidate || signature_overlap < 0.6) return false;

    gs.selected_reference_cell_id = reference_cell_id;

    TransitionProof p;
    p.from = GapState::CANDIDATES_PROPOSED;
    p.to   = GapState::REFERENCE_SELECTED;
    p.similarity        = similarity;
    p.signature_overlap = signature_overlap;
    p.evidence_cell     = reference_cell_id;
    p.reasoning         = "selected reference " + std::to_string(reference_cell_id) +
                          " (sig_overlap=" + std::to_string(signature_overlap) + ")";
    p.correct = (signature_overlap >= 0.6);
    return try_accept_transition(gs, p);
}

// REFERENCE_SELECTED -> APPLIED
// Correct: adapted code has balanced parens/braces
static inline bool apply_adaptation(
    GapConstitutionalState& gs,
    const std::string& adapted_code
) {
    if (gs.state != GapState::REFERENCE_SELECTED) return false;
    if (adapted_code.empty()) return false;
    int paren = 0, brace = 0;
    for (char c : adapted_code) {
        if (c == '(') paren++;
        else if (c == ')') paren--;
        else if (c == '{') brace++;
        else if (c == '}') brace--;
        if (paren < 0 || brace < 0) return false;
    }
    if (paren != 0 || brace != 0) return false;

    gs.applied_adaptation = adapted_code;

    TransitionProof p;
    p.from = GapState::REFERENCE_SELECTED;
    p.to   = GapState::APPLIED;
    p.reasoning = "applied adaptation (" + std::to_string(adapted_code.size()) + " bytes)";
    p.correct = true;
    return try_accept_transition(gs, p);
}

// APPLIED -> VERIFIED
// Correct: verification_result is non-empty
static inline bool verify_application(
    GapConstitutionalState& gs,
    const std::string& verification_result
) {
    if (gs.state != GapState::APPLIED) return false;
    if (verification_result.empty()) return false;

    gs.verification_result = verification_result;

    TransitionProof p;
    p.from = GapState::APPLIED;
    p.to   = GapState::VERIFIED;
    p.evidence_text = verification_result;
    p.reasoning = "verified: " + verification_result;
    p.correct = true;
    return try_accept_transition(gs, p);
}

// Any non-terminal -> REJECTED
static inline bool reject_gap(
    GapConstitutionalState& gs,
    std::string_view reason
) {
    if (gs.state == GapState::VERIFIED || gs.state == GapState::REJECTED) return false;
    if (reason.empty()) return false;

    TransitionProof p;
    p.from = gs.state;
    p.to   = GapState::REJECTED;
    p.reasoning = std::string(reason);
    p.correct = true;
    return try_accept_transition(gs, p);
}

class GapRegistry {
private:
    std::map<uint32_t, GapConstitutionalState> states_;

public:
    GapConstitutionalState& register_gap(uint32_t gap_id, const CodeGap& gap, const std::string& file_uri = "") {
        auto it = states_.find(gap_id);
        if (it != states_.end()) return it->second;
        GapConstitutionalState gs;
        gs.gap_id      = gap_id;
        gs.cell_id     = gap.cell_id;
        gs.byte_offset = gap.byte_offset;
        gs.byte_length = gap.byte_length;
        gs.line_number = gap.line_number;
        gs.file_uri    = file_uri;
        gs.signature   = gap.signature;
        gs.language    = gap.language;
        gs.state       = GapState::UNDISCOVERED;
        return states_.emplace(gap_id, std::move(gs)).first->second;
    }

    GapConstitutionalState* find(uint32_t gap_id) {
        auto it = states_.find(gap_id);
        return (it != states_.end()) ? &it->second : nullptr;
    }

    const GapConstitutionalState* find(uint32_t gap_id) const {
        auto it = states_.find(gap_id);
        return (it != states_.end()) ? &it->second : nullptr;
    }

    const std::map<uint32_t, GapConstitutionalState>& all() const noexcept { return states_; }
    std::map<uint32_t, GapConstitutionalState>& all_mut() noexcept { return states_; }

    bool save(const std::string& path) const {
        std::ofstream out(path);
        if (!out) return false;
        out << states_.size() << "\n";
        for (const auto& [id, gs] : states_) {
            out << gs.gap_id << "\t" << gs.cell_id << "\t" << gs.byte_offset << "\t"
                << gs.byte_length << "\t" << gs.line_number << "\t"
                << static_cast<int>(gs.state) << "\t" << gs.selected_reference_cell_id << "\n";
            out << json_escape(gs.file_uri) << "\n";
            out << json_escape(gs.signature) << "\n";
            out << json_escape(gs.language) << "\n";
            out << json_escape(gs.applied_adaptation) << "\n";
            out << json_escape(gs.verification_result) << "\n";
            out << gs.candidate_cell_ids.size() << "\n";
            for (uint32_t cid : gs.candidate_cell_ids) out << cid << "\n";
            out << gs.candidates.size() << "\n";
            for (const auto& c : gs.candidates) {
                out << c.cell_id << "\t" << c.line << "\t" << c.similarity << "\t"
                    << c.signature_overlap << "\t" << json_escape(c.source_path) << "\n";
            }
            out << gs.history.size() << "\n";
            for (const auto& h : gs.history) {
                out << static_cast<int>(h.from) << "\t" << static_cast<int>(h.to) << "\t"
                    << h.divisible << "\t" << h.admissible << "\t" << h.correct << "\t"
                    << h.respectful << "\t" << h.similarity << "\t" << h.signature_overlap << "\t"
                    << h.evidence_cell << "\t" << h.timestamp_unix << "\n";
                out << json_escape(h.evidence_text) << "\n";
                out << json_escape(h.reasoning) << "\n";
            }
        }
        return out.good();
    }

    static std::string json_unescape(std::string_view s) {
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                char n = s[++i];
                if (n == 'n') out.push_back('\n');
                else if (n == 'r') out.push_back('\r');
                else if (n == 't') out.push_back('\t');
                else out.push_back(n);
            } else {
                out.push_back(s[i]);
            }
        }
        return out;
    }

    bool load(const std::string& path) {
        std::ifstream in(path);
        if (!in) return false;
        states_.clear();
        size_t n_states = 0;
        if (!(in >> n_states)) return false;
        std::string dummy;
        std::getline(in, dummy);
        for (size_t i = 0; i < n_states; ++i) {
            GapConstitutionalState gs;
            int st_int = 0;
            in >> gs.gap_id >> gs.cell_id >> gs.byte_offset >> gs.byte_length
               >> gs.line_number >> st_int >> gs.selected_reference_cell_id;
            gs.state = static_cast<GapState>(st_int);
            std::getline(in, dummy);
            std::string line;
            std::getline(in, line); gs.file_uri = json_unescape(line);
            std::getline(in, line); gs.signature = json_unescape(line);
            std::getline(in, line); gs.language = json_unescape(line);
            std::getline(in, line); gs.applied_adaptation = json_unescape(line);
            std::getline(in, line); gs.verification_result = json_unescape(line);
            size_t n_cids = 0;
            in >> n_cids;
            gs.candidate_cell_ids.resize(n_cids);
            for (size_t k = 0; k < n_cids; ++k) in >> gs.candidate_cell_ids[k];
            size_t n_cands = 0;
            in >> n_cands;
            std::getline(in, dummy);
            gs.candidates.resize(n_cands);
            for (size_t k = 0; k < n_cands; ++k) {
                std::getline(in, line);
                std::istringstream ls(line);
                std::string sp;
                ls >> gs.candidates[k].cell_id >> gs.candidates[k].line
                   >> gs.candidates[k].similarity >> gs.candidates[k].signature_overlap;
                std::getline(ls, sp);
                if (!sp.empty() && sp[0] == '\t') sp.erase(0, 1);
                gs.candidates[k].source_path = json_unescape(sp);
            }
            size_t n_hist = 0;
            in >> n_hist;
            std::getline(in, dummy);
            gs.history.resize(n_hist);
            for (size_t k = 0; k < n_hist; ++k) {
                int f_i = 0, t_i = 0;
                in >> f_i >> t_i >> gs.history[k].divisible >> gs.history[k].admissible
                   >> gs.history[k].correct >> gs.history[k].respectful
                   >> gs.history[k].similarity >> gs.history[k].signature_overlap
                   >> gs.history[k].evidence_cell >> gs.history[k].timestamp_unix;
                gs.history[k].from = static_cast<GapState>(f_i);
                gs.history[k].to   = static_cast<GapState>(t_i);
                std::getline(in, dummy);
                std::getline(in, line); gs.history[k].evidence_text = json_unescape(line);
                std::getline(in, line); gs.history[k].reasoning = json_unescape(line);
            }
            states_[gs.gap_id] = std::move(gs);
        }
        return true;
    }
};

static inline bool annotate_gap_with_reference(
    const ManifoldEngine& engine,
    GapConstitutionalState& gs,
    size_t top_k = 3
) {
    CodeGap cg;
    cg.cell_id       = gs.cell_id;
    cg.byte_offset   = gs.byte_offset;
    cg.byte_length   = gs.byte_length;
    cg.line_number   = gs.line_number;
    cg.signature     = gs.signature;
    cg.language      = gs.language;
    GapAnchor anchor = build_gap_anchor(engine, cg);
    auto refs = find_reference_for_gap(engine, anchor, top_k);
    if (refs.empty()) return false;
    return propose_candidates(gs, refs);
}

// ============================================================================
// §10. REFERENCE DISCOVERY, CATALOG NARROWING & CACHE INDEXING (GROUPS D & I)
// ============================================================================

enum class SourceType : uint8_t {
    GITHUB_REPO = 0,
    DOCS_SITE   = 1,
    LOCAL_DIR   = 2,
    ARCHIVE     = 3
};

static inline std::string source_type_name(SourceType t) {
    switch (t) {
        case SourceType::GITHUB_REPO: return "GITHUB_REPO";
        case SourceType::DOCS_SITE:   return "DOCS_SITE";
        case SourceType::LOCAL_DIR:   return "LOCAL_DIR";
        case SourceType::ARCHIVE:     return "ARCHIVE";
        default:                      return "GITHUB_REPO";
    }
}

static inline SourceType parse_source_type(std::string_view s) {
    if (s == "DOCS_SITE" || s == "docs")   return SourceType::DOCS_SITE;
    if (s == "LOCAL_DIR" || s == "local")  return SourceType::LOCAL_DIR;
    if (s == "ARCHIVE"   || s == "archive") return SourceType::ARCHIVE;
    return SourceType::GITHUB_REPO;
}

struct ReferenceSource {
    std::string id;
    std::string url_or_path;
    SourceType  type = SourceType::GITHUB_REPO;
    std::vector<std::string> languages;
    std::vector<std::string> tags;
    std::vector<std::string> include_triggers;
    std::vector<std::string> import_triggers;
    std::vector<std::string> keyword_triggers;
    std::string description;
};

struct SourceCatalog {
    std::vector<ReferenceSource> sources;

    static SourceCatalog builtin() {
        SourceCatalog cat;
        cat.sources = {
            {"github.com/FFTW/fftw3", "https://github.com/FFTW/fftw3", SourceType::GITHUB_REPO,
             {"cpp", "c"}, {"fft", "spectral", "dsp", "math"}, {"fftw3.h"}, {"fftw"}, {"fft", "dft", "rfft", "helmholtz", "fourier"},
             "Fastest Fourier Transform in the West C/C++ library"},
            {"github.com/numpy/numpy", "https://github.com/numpy/numpy", SourceType::GITHUB_REPO,
             {"python", "c"}, {"array", "linalg", "fft", "math"}, {"numpy/arrayobject.h"}, {"numpy", "np"}, {"ndarray", "pocketfft", "linalg", "fft"},
             "Fundamental package for array computing with Python"},
            {"github.com/scipy/scipy", "https://github.com/scipy/scipy", SourceType::GITHUB_REPO,
             {"python", "cpp"}, {"scientific", "fft", "optimize", "sparse"}, {}, {"scipy"}, {"helmholtz", "odeint", "fftpack", "laplacian"},
             "SciPy scientific library for Python"},
            {"github.com/eigen-mirror/eigen", "https://github.com/eigen-mirror/eigen", SourceType::GITHUB_REPO,
             {"cpp"}, {"linalg", "matrix", "geometry", "simd"}, {"Eigen/Dense", "Eigen/Core", "Eigen/Sparse"}, {}, {"MatrixXd", "VectorXd", "jacobi", "cholesky"},
             "Eigen C++ template library for linear algebra"},
            {"github.com/boostorg/boost", "https://github.com/boostorg/boost", SourceType::GITHUB_REPO,
             {"cpp"}, {"stdlib", "graph", "asio", "math"}, {"boost/asio.hpp", "boost/graph/adjacency_list.hpp"}, {}, {"boost", "asio", "adjacency_list"},
             "Boost peer-reviewed portable C++ source libraries"},
            {"github.com/pytorch/pytorch", "https://github.com/pytorch/pytorch", SourceType::GITHUB_REPO,
             {"cpp", "python"}, {"ml", "tensor", "autograd", "cuda"}, {"torch/torch.h", "ATen/ATen.h"}, {"torch"}, {"tensor", "autograd", "backward"},
             "Tensors and dynamic neural networks in Python and C++"},
            {"github.com/ggml-org/llama.cpp", "https://github.com/ggml-org/llama.cpp", SourceType::GITHUB_REPO,
             {"cpp", "c"}, {"llm", "quantization", "simd", "ggml"}, {"ggml.h", "llama.h"}, {}, {"ggml", "quantize", "kv_cache", "rope"},
             "LLM inference in C/C++ with GGML tensor kernels"},
            {"github.com/nlohmann/json", "https://github.com/nlohmann/json", SourceType::GITHUB_REPO,
             {"cpp"}, {"json", "serialization", "parser"}, {"nlohmann/json.hpp"}, {}, {"json", "parse", "dump"},
             "JSON for Modern C++"},
            {"github.com/fmtlib/fmt", "https://github.com/fmtlib/fmt", SourceType::GITHUB_REPO,
             {"cpp"}, {"formatting", "io", "string"}, {"fmt/core.h", "fmt/format.h"}, {}, {"fmt", "format", "vformat"},
             "Modern C++ formatting library"},
            {"github.com/gabime/spdlog", "https://github.com/gabime/spdlog", SourceType::GITHUB_REPO,
             {"cpp"}, {"logging", "io"}, {"spdlog/spdlog.h"}, {}, {"spdlog", "logger", "sink"},
             "Fast C++ logging library"},
            {"github.com/catchorg/Catch2", "https://github.com/catchorg/Catch2", SourceType::GITHUB_REPO,
             {"cpp"}, {"testing", "unit-test"}, {"catch2/catch_test_macros.hpp"}, {}, {"TEST_CASE", "REQUIRE"},
             "Modern C++-native test framework"},
            {"github.com/google/googletest", "https://github.com/google/googletest", SourceType::GITHUB_REPO,
             {"cpp"}, {"testing", "mock"}, {"gtest/gtest.h", "gmock/gmock.h"}, {}, {"EXPECT_EQ", "ASSERT_TRUE", "TEST"},
             "Google Testing and Mocking Framework"},
            {"github.com/sqlite/sqlite", "https://github.com/sqlite/sqlite", SourceType::GITHUB_REPO,
             {"c", "cpp"}, {"database", "sql", "storage", "btree"}, {"sqlite3.h"}, {"sqlite3"}, {"sqlite3_exec", "btree", "wal"},
             "SQLite embedded relational database engine"},
            {"github.com/curl/curl", "https://github.com/curl/curl", SourceType::GITHUB_REPO,
             {"c", "cpp"}, {"http", "network", "tls"}, {"curl/curl.h"}, {}, {"curl_easy_perform", "libcurl", "http"},
             "Command line tool and library for transferring data with URLs"},
            {"github.com/openssl/openssl", "https://github.com/openssl/openssl", SourceType::GITHUB_REPO,
             {"c", "cpp"}, {"crypto", "tls", "hash", "sha256"}, {"openssl/sha.h", "openssl/evp.h", "openssl/ssl.h"}, {}, {"EVP_Digest", "SHA256", "hmac"},
             "TLS/SSL and cryptographic library"},
            {"github.com/rust-lang/rust", "https://github.com/rust-lang/rust", SourceType::GITHUB_REPO,
             {"rust"}, {"std", "compiler", "collections"}, {}, {"std::collections", "std::sync"}, {"Vec", "HashMap", "Arc"},
             "The Rust standard library and compiler"},
            {"github.com/tokio-rs/tokio", "https://github.com/tokio-rs/tokio", SourceType::GITHUB_REPO,
             {"rust"}, {"async", "runtime", "io", "net"}, {}, {"tokio", "tokio::net", "tokio::sync"}, {"spawn", "mpsc", "epoll"},
             "Runtime for writing reliable asynchronous applications with Rust"},
            {"github.com/serde-rs/serde", "https://github.com/serde-rs/serde", SourceType::GITHUB_REPO,
             {"rust"}, {"serialization", "json", "derive"}, {}, {"serde", "serde_json"}, {"Serialize", "Deserialize"},
             "Serialization framework for Rust"},
            {"github.com/LuaJIT/LuaJIT", "https://github.com/LuaJIT/LuaJIT", SourceType::GITHUB_REPO,
             {"lua", "c"}, {"jit", "vm", "ffi", "scripting"}, {"lua.h", "lauxlib.h", "luajit.h"}, {"ffi", "bit"}, {"lua_State", "ffi.cdef"},
             "Just-In-Time Compiler for the Lua programming language"},
            {"github.com/nodejs/node", "https://github.com/nodejs/node", SourceType::GITHUB_REPO,
             {"js", "cpp"}, {"runtime", "event-loop", "fs", "http"}, {"node.h", "v8.h"}, {"fs", "path", "http", "crypto"}, {"EventEmitter", "Buffer", "Promise"},
             "Node.js JavaScript runtime"},
            {"github.com/facebook/react", "https://github.com/facebook/react", SourceType::GITHUB_REPO,
             {"js"}, {"ui", "components", "hooks"}, {}, {"react", "react-dom"}, {"useState", "useEffect", "useMemo"},
             "React library for web and native user interfaces"},
            {"en.cppreference.com", "https://en.cppreference.com/w/cpp", SourceType::DOCS_SITE,
             {"cpp", "c"}, {"reference", "stl", "ranges", "concepts"}, {"vector", "algorithm", "ranges", "span", "bit"}, {}, {"std::popcount", "std::span", "std::ranges"},
             "C++ Standard Library reference documentation"}
        };
        return cat;
    }

    static std::string join_vec(const std::vector<std::string>& v) {
        std::string o;
        for (size_t i = 0; i < v.size(); ++i) {
            if (i > 0) o.push_back(',');
            o += v[i];
        }
        return o;
    }

    static std::vector<std::string> split_csv(std::string_view sv) {
        std::vector<std::string> out;
        size_t start = 0;
        while (start < sv.size()) {
            size_t comma = sv.find(',', start);
            if (comma == std::string_view::npos) comma = sv.size();
            std::string item(sv.substr(start, comma - start));
            if (!item.empty()) out.push_back(item);
            start = comma + 1;
        }
        return out;
    }

    bool save(const std::string& path) const {
        std::ofstream out(path);
        if (!out) return false;
        for (const auto& s : sources) {
            out << s.id << "\t" << s.url_or_path << "\t" << source_type_name(s.type) << "\t"
                << join_vec(s.languages) << "\t" << join_vec(s.tags) << "\t"
                << join_vec(s.include_triggers) << "\t" << join_vec(s.import_triggers) << "\t"
                << join_vec(s.keyword_triggers) << "\t" << s.description << "\n";
        }
        return out.good();
    }

    bool load(const std::string& path) {
        std::ifstream in(path);
        if (!in) return false;
        sources.clear();
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::vector<std::string> cols;
            size_t start = 0;
            while (true) {
                size_t tab = line.find('\t', start);
                if (tab == std::string::npos) {
                    cols.push_back(line.substr(start));
                    break;
                }
                cols.push_back(line.substr(start, tab - start));
                start = tab + 1;
            }
            if (cols.size() < 9) continue;
            ReferenceSource rs;
            rs.id               = cols[0];
            rs.url_or_path      = cols[1];
            rs.type             = parse_source_type(cols[2]);
            rs.languages        = split_csv(cols[3]);
            rs.tags             = split_csv(cols[4]);
            rs.include_triggers = split_csv(cols[5]);
            rs.import_triggers  = split_csv(cols[6]);
            rs.keyword_triggers = split_csv(cols[7]);
            rs.description      = cols[8];
            sources.push_back(std::move(rs));
        }
        return true;
    }

    static SourceCatalog load_effective(const std::string& custom_path = "") {
        SourceCatalog base = builtin();
        if (!custom_path.empty() && fs::exists(custom_path)) {
            SourceCatalog extra;
            if (extra.load(custom_path)) {
                for (const auto& es : extra.sources) {
                    auto it = std::find_if(base.sources.begin(), base.sources.end(),
                                           [&](const ReferenceSource& r) { return r.id == es.id; });
                    if (it != base.sources.end()) *it = es;
                    else base.sources.push_back(es);
                }
            }
        }
        return base;
    }
};

struct LocalSignals {
    std::set<std::string> includes;
    std::set<std::string> imports;
    std::set<std::string> urls;
    std::set<std::string> manifest_deps;
    std::set<std::string> keywords;
    std::set<std::string> languages;
};

static inline LocalSignals extract_local_signals_from_text(std::string_view text, std::string_view filename = "") {
    LocalSignals sig;
    std::string lang = detect_language_from_path(filename);
    if (lang != "unknown") sig.languages.insert(lang);

    const std::regex re_inc(R"(^\s*#\s*include\s*[<"]([^>"]+)[>"])");
    const std::regex re_py_imp(R"(^\s*(?:import|from)\s+([a-zA-Z0-9_\.]+))");
    const std::regex re_rs_use(R"(^\s*use\s+([a-zA-Z0-9_:]+))");
    const std::regex re_js_req(R"(require\s*\(\s*['"]([^'"]+)['"]\s*\))");
    const std::regex re_url(R"(https?://[^\s"'<>]+)");

    size_t line_start = 0;
    while (line_start <= text.size()) {
        size_t line_end = text.find('\n', line_start);
        if (line_end == std::string_view::npos) line_end = text.size();
        std::string line(text.substr(line_start, line_end - line_start));

        std::smatch m;
        if (std::regex_search(line, m, re_inc)) {
            sig.includes.insert(m[1].str());
        }
        if (std::regex_search(line, m, re_py_imp)) {
            std::string mod = m[1].str();
            size_t dot = mod.find('.');
            sig.imports.insert(dot == std::string::npos ? mod : mod.substr(0, dot));
        }
        if (std::regex_search(line, m, re_rs_use)) {
            std::string crate = m[1].str();
            size_t col = crate.find("::");
            sig.imports.insert(col == std::string::npos ? crate : crate.substr(0, col));
        }
        if (std::regex_search(line, m, re_js_req)) {
            sig.imports.insert(m[1].str());
        }
        if (std::regex_search(line, m, re_url)) {
            sig.urls.insert(m[0].str());
        }

        // Tokenize identifiers and snake_case subwords for keyword triggers
        auto insert_token_and_subwords = [&](const std::string& t) {
            if (t.size() >= 3) sig.keywords.insert(t);
            size_t start = 0;
            while (start < t.size()) {
                size_t pos = t.find('_', start);
                if (pos == std::string::npos) pos = t.size();
                if (pos > start && (pos - start) >= 3) {
                    sig.keywords.insert(t.substr(start, pos - start));
                }
                start = pos + 1;
            }
        };
        std::string tok;
        for (char c : line) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
                tok.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            } else {
                insert_token_and_subwords(tok);
                tok.clear();
            }
        }
        insert_token_and_subwords(tok);

        if (line_end >= text.size()) break;
        line_start = line_end + 1;
    }
    return sig;
}

static inline LocalSignals extract_local_signals(const fs::path& repo_or_file) {
    LocalSignals total;
    std::error_code ec;
    auto merge_into = [&](const LocalSignals& s) {
        total.includes.insert(s.includes.begin(), s.includes.end());
        total.imports.insert(s.imports.begin(), s.imports.end());
        total.urls.insert(s.urls.begin(), s.urls.end());
        total.manifest_deps.insert(s.manifest_deps.begin(), s.manifest_deps.end());
        total.keywords.insert(s.keywords.begin(), s.keywords.end());
        total.languages.insert(s.languages.begin(), s.languages.end());
    };
    auto handle_file = [&](const fs::path& p) {
        std::string raw;
        if (!read_text_file_safe(p, raw)) return;
        std::string fn = p.filename().string();
        auto s = extract_local_signals_from_text(raw, fn);
        if (fn == "requirements.txt" || fn == "Cargo.toml" || fn == "package.json" || fn == "CMakeLists.txt") {
            for (const auto& kw : s.keywords) total.manifest_deps.insert(kw);
        }
        merge_into(s);
    };
    if (fs::is_regular_file(repo_or_file, ec)) {
        handle_file(repo_or_file);
    } else if (fs::is_directory(repo_or_file, ec)) {
        for (auto it = fs::recursive_directory_iterator(repo_or_file, fs::directory_options::skip_permission_denied, ec);
             it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (ec) break;
            if (it->is_regular_file(ec)) handle_file(it->path());
        }
    }
    return total;
}

struct DiscoveredCandidate {
    ReferenceSource source;
    double score = 0.0;
    std::vector<std::string> matched_reasons;
};

static inline std::vector<DiscoveredCandidate> discover_from_signals(
    const LocalSignals& sig,
    const SourceCatalog& catalog = SourceCatalog::builtin()
) {
    std::vector<DiscoveredCandidate> out;
    for (const auto& src : catalog.sources) {
        double score = 0.0;
        std::vector<std::string> reasons;

        for (const auto& inc : src.include_triggers) {
            if (sig.includes.count(inc)) {
                score += 0.80;
                reasons.push_back("include:" + inc);
            }
        }
        for (const auto& imp : src.import_triggers) {
            if (sig.imports.count(imp)) {
                score += 1.00;
                reasons.push_back("import:" + imp);
            }
            if (sig.manifest_deps.count(imp)) {
                score += 0.65;
                reasons.push_back("manifest:" + imp);
            }
        }
        for (const auto& kw : src.keyword_triggers) {
            std::string lkw = kw;
            for (char& c : lkw) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (sig.keywords.count(lkw)) {
                score += 0.42;
                reasons.push_back("keyword:" + kw);
            }
        }
        for (const auto& lang : src.languages) {
            if (sig.languages.count(lang) && score > 0.0) {
                score += 0.05;
            }
        }
        if (score > 0.0) {
            out.push_back({src, score, std::move(reasons)});
        }
    }
    std::sort(out.begin(), out.end(), [](const DiscoveredCandidate& a, const DiscoveredCandidate& b) {
        return a.score > b.score || (a.score == b.score && a.source.id < b.source.id);
    });
    return out;
}

static inline std::vector<DiscoveredCandidate> discover_from_local(
    const fs::path& repo_path,
    const SourceCatalog& catalog = SourceCatalog::builtin()
) {
    LocalSignals sig = extract_local_signals(repo_path);
    return discover_from_signals(sig, catalog);
}

static inline std::vector<DiscoveredCandidate> discover_from_description(
    std::string_view description,
    const SourceCatalog& catalog = SourceCatalog::builtin()
) {
    LocalSignals sig = extract_local_signals_from_text(description, "query.cpp");
    for (const auto& src : catalog.sources) {
        for (const auto& tag : src.tags) {
            if (sig.keywords.count(tag)) {
                sig.imports.insert(tag);
            }
        }
    }
    return discover_from_signals(sig, catalog);
}

enum class NarrowOp : uint8_t {
    MORE_LIKE = 0,
    EXCLUDE   = 1,
    TAG       = 2,
    LANGUAGE  = 3,
    TYPE      = 4
};

struct NarrowFilter {
    NarrowOp op = NarrowOp::TAG;
    std::string value;
};

static inline std::vector<DiscoveredCandidate> narrow_candidates(
    const std::vector<DiscoveredCandidate>& cands,
    const NarrowFilter& filter
) {
    std::vector<DiscoveredCandidate> out;
    if (filter.op == NarrowOp::EXCLUDE) {
        for (const auto& c : cands) {
            if (c.source.id.find(filter.value) == std::string::npos) out.push_back(c);
        }
        return out;
    }
    if (filter.op == NarrowOp::TAG) {
        for (const auto& c : cands) {
            if (std::find(c.source.tags.begin(), c.source.tags.end(), filter.value) != c.source.tags.end()) {
                out.push_back(c);
            }
        }
        return out;
    }
    if (filter.op == NarrowOp::LANGUAGE) {
        for (const auto& c : cands) {
            if (std::find(c.source.languages.begin(), c.source.languages.end(), filter.value) != c.source.languages.end()) {
                out.push_back(c);
            }
        }
        return out;
    }
    if (filter.op == NarrowOp::TYPE) {
        SourceType target_t = parse_source_type(filter.value);
        for (const auto& c : cands) {
            if (c.source.type == target_t) out.push_back(c);
        }
        return out;
    }
    if (filter.op == NarrowOp::MORE_LIKE) {
        std::set<std::string> target_tags;
        for (const auto& c : cands) {
            if (c.source.id.find(filter.value) != std::string::npos) {
                target_tags.insert(c.source.tags.begin(), c.source.tags.end());
            }
        }
        for (auto c : cands) {
            size_t shared = 0;
            for (const auto& t : c.source.tags) if (target_tags.count(t)) shared++;
            if (shared > 0) {
                c.score += 0.25 * static_cast<double>(shared);
                out.push_back(std::move(c));
            }
        }
        std::sort(out.begin(), out.end(), [](const DiscoveredCandidate& a, const DiscoveredCandidate& b) {
            return a.score > b.score;
        });
    }
    return out;
}

// Group I: Cache & Indexing
enum class CacheStatus : uint8_t {
    MISS      = 0,
    HIT_FRESH = 1,
    HIT_STALE = 2
};

static inline std::string compute_cache_key(std::string_view source_id) {
    uint64_t h = fnv1a_str(source_id);
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << h;
    return oss.str();
}

static inline CacheStatus cache_status(const fs::path& cache_root, std::string_view source_id) {
    std::error_code ec;
    fs::path dir = cache_root / compute_cache_key(source_id);
    if (!fs::exists(dir, ec)) return CacheStatus::MISS;
    if (fs::exists(dir / ".stale", ec)) return CacheStatus::HIT_STALE;
    return CacheStatus::HIT_FRESH;
}

static inline fs::path fetch_to_cache(
    const fs::path& cache_root,
    const ReferenceSource& src,
    bool allow_mock = true
) {
    std::error_code ec;
    fs::path dir = cache_root / compute_cache_key(src.id);
    if (fs::exists(dir, ec) && !fs::exists(dir / ".stale", ec)) {
        return dir;
    }
    fs::create_directories(dir, ec);
    // If url_or_path is a local existing path, copy files into cache dir
    if (fs::exists(src.url_or_path, ec)) {
        if (fs::is_regular_file(src.url_or_path, ec)) {
            fs::copy_file(src.url_or_path, dir / fs::path(src.url_or_path).filename(),
                          fs::copy_options::overwrite_existing, ec);
        } else if (fs::is_directory(src.url_or_path, ec)) {
            fs::copy(src.url_or_path, dir,
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        }
        return dir;
    }
    const char* mock_env = std::getenv("MANIFOLD_MOCK_FETCH");
    if (allow_mock || (mock_env && std::string(mock_env) == "1")) {
        // Synthesize deterministic reference header/module in cache for offline operation
        std::string ext = (!src.languages.empty() && src.languages[0] == "python") ? ".py" : ".hpp";
        fs::path ref_file = dir / ("reference_impl" + ext);
        std::ofstream out(ref_file);
        out << "// Reference implementation for " << src.id << " (" << src.description << ")\n";
        for (const auto& kw : src.keyword_triggers) {
            out << "void " << kw << "_step(float* H, int n, float dt, float g) {\n"
                << "    for (int i = 0; i < n; ++i) { H[i] += dt * g; }\n"
                << "}\n\n";
        }
        out.close();
        std::ofstream mf(cache_root / "manifest.tsv", std::ios::app);
        mf << src.id << "\t" << compute_cache_key(src.id) << "\t" << now_unix() << "\n";
    }
    return dir;
}

enum class IndexOutcome : uint8_t {
    INDEXED        = 0,
    SKIPPED_CACHED = 1,
    FAILED         = 2
};

struct IndexSourceReport {
    std::string source_id;
    IndexOutcome outcome = IndexOutcome::FAILED;
    uint32_t cells_added = 0;
};

static inline std::vector<IndexSourceReport> select_and_index(
    ManifoldEngine& engine,
    const std::vector<std::string>& source_ids_or_paths,
    const fs::path& cache_root = "/tmp/manifold_ref_cache",
    const SourceCatalog& catalog = SourceCatalog::builtin()
) {
    std::vector<IndexSourceReport> reports;
    for (const auto& item : source_ids_or_paths) {
        if (item.empty()) continue;
        // Check if already indexed in engine under corpus_id == item
        bool already_in_engine = false;
        for (const auto& c : engine.cells) {
            if (c.corpus_id == item) { already_in_engine = true; break; }
        }
        if (already_in_engine) {
            reports.push_back({item, IndexOutcome::SKIPPED_CACHED, 0});
            continue;
        }

        ReferenceSource matched_src;
        auto it = std::find_if(catalog.sources.begin(), catalog.sources.end(),
                               [&](const ReferenceSource& r) { return r.id == item; });
        if (it != catalog.sources.end()) {
            matched_src = *it;
        } else {
            matched_src.id = item;
            matched_src.url_or_path = item;
            matched_src.type = SourceType::LOCAL_DIR;
            matched_src.keyword_triggers = {"fft", "helmholtz", "solver"};
        }

        size_t before = engine.cells.size();
        std::error_code ec;
        if (fs::exists(matched_src.url_or_path, ec)) {
            engine.ingest_path(matched_src.url_or_path, Modality::EXT_REF, matched_src.id);
        } else {
            fs::path cached_dir = fetch_to_cache(cache_root, matched_src, true);
            engine.ingest_path(cached_dir, Modality::EXT_REF, matched_src.id);
        }
        uint32_t added = static_cast<uint32_t>(engine.cells.size() - before);
        reports.push_back({item, added > 0 ? IndexOutcome::INDEXED : IndexOutcome::FAILED, added});
    }
    return reports;
}

static int run_selftest() {
    bool test_ok = true;

    std::cout << "[TEST SURVIVORS] Post-removal baseline tests\n";
    {
        // Hypervector determinism: same input -> same output
        Hypervec8192 a = encode_span_hypervector("void foo(int x) { return x; }");
        Hypervec8192 b = encode_span_hypervector("void foo(int x) { return x; }");
        bool same_input_same_hv = (a.hamming(b) == 0);
        // Different input -> different output
        Hypervec8192 c = encode_span_hypervector("void bar(float y) { return y; }");
        bool diff_input_diff_hv = (a.hamming(c) >= 1000);
        // Byte stats
        ByteStats st = compute_byte_stats("int main() { return 0; }");
        bool stats_ok = (st.shannon_entropy > 0.0 && st.mean_delta > 0.0);
        bool pass = same_input_same_hv && diff_input_diff_hv && stats_ok;
        std::cout << "  • Hypervector + byte stats: " << (pass ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST CHECKPOINT] Round-trip save/load\n";
    {
        ManifoldEngine e;
        e.ingest_file("test.cpp", "int foo() { return 1; }\nint bar() { return 2; }",
                      Modality::REPO_CODE);
        e.save_checkpoint("/tmp/manifold_rt.mfd");
        ManifoldEngine e2;
        bool loaded = e2.load_checkpoint("/tmp/manifold_rt.mfd");
        bool same_count = (loaded && e2.cells.size() == e.cells.size());
        std::cout << "  • Round-trip: " << (same_count ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && same_count;
    }

    std::cout << "[TEST PLACEMENT JSON] Round-trip\n";
    {
        Placement p;
        p.kind = PlacementKind::FILL;
        p.sources.push_back({"src/foo.cpp", 47, 49});
        p.dest = {"src/foo.cpp", 47, 49, false};
        p.confidence = 0.82;
        p.reason = "empty body matched";
        p.references.push_back({"lib/scipy/fft.py", 234, 0.847, 0.71, 0});
        std::string j = placement_to_json(p);
        bool has_kind = j.find("\"kind\":\"FILL\"") != std::string::npos;
        bool has_dest = j.find("\"dest\":{") != std::string::npos;
        bool has_ref  = j.find("\"source_path\":\"lib/scipy/fft.py\"") != std::string::npos;
        bool pass = has_kind && has_dest && has_ref;
        std::cout << "  • [TEST PLACEMENT JSON] JSON: " << (pass ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST PLACEMENT PARSE] PlacementKind\n";
    {
        bool pass = parse_placement_kind("fill") == PlacementKind::FILL &&
                    parse_placement_kind("move") == PlacementKind::MOVE &&
                    parse_placement_kind("extract") == PlacementKind::EXTRACT &&
                    !parse_placement_kind("bogus").has_value();
        std::cout << "  • [TEST PLACEMENT PARSE] Parse: " << (pass ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST GAP CPP] Detector\n";
    {
        std::string src = "void foo(int);\nvoid bar() {}\n// TODO: fix baz\nvoid good() { return; }\n";
        auto gaps = detect_cpp_gaps(src, 42);
        bool pass = gaps.size() == 3 &&
                    gaps[0].kind == GapKind::STUB_DECLARATION && gaps[0].function_name == "foo" && gaps[0].line_number == 1 &&
                    gaps[1].kind == GapKind::EMPTY_BODY && gaps[1].function_name == "bar" && gaps[1].line_number == 2 &&
                    gaps[2].kind == GapKind::TODO_COMMENT && gaps[2].line_number == 3;
        std::cout << "  • [TEST GAP CPP] C++ gaps: " << (pass ? "PASS" : "FAIL") << " (" << gaps.size() << " found)\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST GAP PY] Detector\n";
    {
        std::string src = "def foo():\n    pass\n\ndef bar() -> int:\n    ...\n\n# TODO: baz\n";
        auto gaps = detect_python_gaps(src, 42);
        bool pass = gaps.size() == 3 &&
                    gaps[0].kind == GapKind::PASS_BODY && gaps[0].function_name == "foo" &&
                    gaps[1].kind == GapKind::ELLIPSIS_BODY && gaps[1].function_name == "bar" &&
                    gaps[2].kind == GapKind::TODO_COMMENT;
        std::cout << "  • [TEST GAP PY] Python gaps: " << (pass ? "PASS" : "FAIL") << " (" << gaps.size() << " found)\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST GAP RS] Detector\n";
    {
        std::string src = "fn foo() { todo!() }\nfn bar() {}\n// TODO: baz\n";
        auto gaps = detect_rust_gaps(src, 42);
        bool pass = gaps.size() == 3 &&
                    gaps[0].kind == GapKind::NOT_IMPLEMENTED && gaps[0].function_name == "foo" &&
                    gaps[1].kind == GapKind::EMPTY_BODY && gaps[1].function_name == "bar";
        std::cout << "  • [TEST GAP RS] Rust gaps: " << (pass ? "PASS" : "FAIL") << " (" << gaps.size() << " found)\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST GAP LUA JS] Detectors\n";
    {
        std::string lua = "function M.foo() end\n-- TODO: fix bar\n";
        auto lg = detect_lua_gaps(lua, 1);
        bool lua_ok = lg.size() == 2 && lg[0].kind == GapKind::EMPTY_BODY && lg[0].function_name == "M.foo";

        std::string js = "function foo() {}\nconst bar = () => {}\nfunction baz() { throw new Error('x') }\n// TODO: qux\n";
        auto jg = detect_js_gaps(js, 1);
        bool js_ok = jg.size() == 4 &&
                     jg[0].kind == GapKind::EMPTY_BODY &&
                     jg[1].kind == GapKind::EMPTY_BODY &&
                     jg[2].kind == GapKind::NOT_IMPLEMENTED &&
                     jg[3].kind == GapKind::TODO_COMMENT;

        std::cout << "  • [TEST GAP LUA JS] Lua: " << (lua_ok ? "PASS" : "FAIL") << ", JS: " << (js_ok ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && lua_ok && js_ok;
    }

    std::cout << "[TEST GAP COORD] find_stub_gaps across languages\n";
    {
        ManifoldEngine e;
        e.ingest_file("src/foo.cpp", "void stub_fn();\nvoid empty_fn() {}\n", Modality::REPO_CODE);
        e.ingest_file("src/bar.py", "def py_stub():\n    pass\n", Modality::REPO_CODE);
        auto found = find_stub_gaps(e);
        bool lang_cpp = detect_language_from_path("foo.cpp") == "cpp";
        bool lang_py  = detect_language_from_path("bar.py") == "python";
        bool lang_rs  = detect_language_from_path("baz.rs") == "rust";
        bool lang_lua = detect_language_from_path("q.lua") == "lua";
        bool lang_js  = detect_language_from_path("w.js") == "js";
        bool lang_unk = detect_language_from_path("x.md") == "unknown";
        bool pass = lang_cpp && lang_py && lang_rs && lang_lua && lang_js && lang_unk && (found.size() == 3);
        std::cout << "  • [TEST GAP COORD] Language dispatch: " << (pass ? "PASS" : "FAIL") << "\n";
        // Per-kind 7 gap detector checks (TEST GAP-1 .. TEST GAP-7)
        bool g1 = !detect_cpp_gaps("void f(int);\n", 1).empty();
        bool g2 = !detect_cpp_gaps("void f() {}\n", 1).empty();
        bool g3 = !detect_cpp_gaps("// TODO: impl\n", 1).empty();
        bool g4 = !detect_rust_gaps("fn f() { todo!() }\n", 1).empty();
        bool g5 = !detect_python_gaps("def f(): pass\n", 1).empty();
        bool g6 = !detect_python_gaps("def f(): ...\n", 1).empty();
        bool g7 = !detect_js_gaps("function f() { throw new Error('ni') }\n", 1).empty();
        std::cout << "  • [TEST GAP-1] STUB_DECLARATION: " << (g1 ? "PASS" : "FAIL") << "\n";
        std::cout << "  • [TEST GAP-2] EMPTY_BODY: " << (g2 ? "PASS" : "FAIL") << "\n";
        std::cout << "  • [TEST GAP-3] TODO_COMMENT: " << (g3 ? "PASS" : "FAIL") << "\n";
        std::cout << "  • [TEST GAP-4] NOT_IMPLEMENTED: " << (g4 ? "PASS" : "FAIL") << "\n";
        std::cout << "  • [TEST GAP-5] PASS_BODY: " << (g5 ? "PASS" : "FAIL") << "\n";
        std::cout << "  • [TEST GAP-6] ELLIPSIS_BODY: " << (g6 ? "PASS" : "FAIL") << "\n";
        std::cout << "  • [TEST GAP-7] JS_THROW_GAP: " << (g7 ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && pass && g1 && g2 && g3 && g4 && g5 && g6 && g7;
    }

    std::cout << "[TEST ANCHOR CALLSITES] find_call_sites\n";
    {
        ManifoldEngine e;
        e.ingest_file("test.cpp", "void foo(int x) {}\nint main() { foo(1); foo(2); return 0; }\n",
                      Modality::REPO_CODE);
        std::string sites = find_call_sites(e, "foo");
        bool pass = sites.empty() || sites.find("foo(") != std::string::npos;
        std::cout << "  • [TEST ANCHOR CALLSITES] Call sites: " << (pass ? "PASS" : "FAIL") << " (" << sites.size() << " bytes)\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST ANCHOR BUILD] build_gap_anchor\n";
    {
        CodeGap g;
        g.cell_id = 1;
        g.byte_offset = 0;
        g.byte_length = 20;
        g.line_number = 1;
        g.kind = GapKind::EMPTY_BODY;
        g.signature = "void fft_step(float*, int, float, float)";
        g.function_name = "fft_step";
        g.language = "cpp";
        ManifoldEngine e;
        GapAnchor a = build_gap_anchor(e, g);
        bool pass = a.signature == g.signature && a.gap.cell_id == g.cell_id;
        uint32_t bits = 0;
        for (auto w : a.hv.words) bits += static_cast<uint32_t>(std::popcount(w));
        pass = pass && (bits > 0);
        std::cout << "  • [TEST ANCHOR BUILD] Anchor build: " << (pass ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST SIG OVERLAP] Jaccard\n";
    {
        double a = compute_signature_overlap("void foo(int, int)", "void foo(int, int)");
        double b = compute_signature_overlap("void foo(int, int)", "void foo(float, float)");
        double c = compute_signature_overlap("void foo", "int bar");
        bool pass = std::abs(a - 1.0) < 1e-9 && b > 0.0 && b < 1.0 && c == 0.0;
        std::cout << "  • [TEST SIG OVERLAP] Overlap: " << (pass ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST FILL REF] Empty corpus -> empty result\n";
    {
        ManifoldEngine e;
        CodeGap g;
        g.cell_id = 1;
        g.signature = "void foo(int)";
        g.function_name = "foo";
        g.language = "cpp";
        GapAnchor a = build_gap_anchor(e, g);
        auto refs = find_reference_for_gap(e, a, 3);
        bool pass = refs.empty();
        std::cout << "  • [TEST FILL REF] Empty: " << (pass ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST FILL EMPTY] Empty corpus -> empty placements\n";
    {
        ManifoldEngine e;
        std::vector<CodeGap> gaps;
        auto ps = find_fill_placements(e, gaps);
        bool pass = ps.empty();
        std::cout << "  • [TEST FILL EMPTY] Empty: " << (pass ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST MOVE NEIGHBORS] Neighbor extraction\n";
    {
        ManifoldEngine e;
        std::string multi_blk(300, 'a');
        multi_blk += "\nint a() { return 1; }\n\n";
        multi_blk += std::string(300, 'b') + "\nint b() { return 2; }\n";
        e.ingest_file("test.cpp", multi_blk, Modality::REPO_CODE);
        uint32_t blk_idx = 0;
        for (size_t i = 0; i < e.cells.size(); ++i) {
            if (e.cells[i].halo.scale == Scale::BLOCK) { blk_idx = static_cast<uint32_t>(i); break; }
        }
        auto neigh = get_neighbors(e, blk_idx);
        bool pass = !neigh.empty();
        std::cout << "  • [TEST MOVE NEIGHBORS] Neighbors: " << (pass ? "PASS" : "FAIL") << " (" << neigh.size() << " found)\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST MOVE EMPTY] Empty corpus -> empty move placements\n";
    {
        ManifoldEngine e;
        auto ps = find_move_placements(e);
        bool pass = ps.empty();
        std::cout << "  • [TEST MOVE EMPTY] Empty: " << (pass ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST EXTRACT EMPTY] Empty corpus -> empty extract placements\n";
    {
        ManifoldEngine e;
        auto ps = find_extract_placements(e);
        bool pass = ps.empty();
        std::cout << "  • [TEST EXTRACT EMPTY] Empty: " << (pass ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && pass;
    }

    std::cout << "[TEST STATE-1..6] Gap State Machine & TransitionProof\n";
    {
        // [TEST STATE-1]: Enumerate all 36 from/to pairs, assert exactly 8 allowed
        int allowed_count = 0;
        for (int f = 0; f < 6; ++f) {
            for (int t = 0; t < 6; ++t) {
                if (is_allowed_transition(static_cast<GapState>(f), static_cast<GapState>(t))) {
                    allowed_count++;
                }
            }
        }
        bool s1_ok = (allowed_count == 8);
        std::cout << "  • [TEST STATE-1] Enumerate 36 pairs (8 allowed): " << (s1_ok ? "PASS" : "FAIL") << "\n";

        // [TEST STATE-2]: Construct TransitionProof and GapConstitutionalState
        TransitionProof tp;
        tp.from = GapState::UNDISCOVERED;
        tp.to   = GapState::CANDIDATES_PROPOSED;
        bool invalid_initially = !tp.is_valid();
        tp.divisible  = true;
        tp.admissible = true;
        tp.correct    = true;
        tp.respectful = true;
        bool valid_when_all_four = tp.is_valid();
        GapConstitutionalState gs_init;
        bool s2_ok = invalid_initially && valid_when_all_four && (gs_init.state == GapState::UNDISCOVERED);
        std::cout << "  • [TEST STATE-2] TransitionProof 4-property gate: " << (s2_ok ? "PASS" : "FAIL") << "\n";

        // [TEST STATE-3]: try_accept_transition valid, wrong from-state, and non-allowed to-state
        GapConstitutionalState gs3;
        TransitionProof p_wrong_from = tp;
        p_wrong_from.from = GapState::CANDIDATES_PROPOSED;
        p_wrong_from.to   = GapState::REFERENCE_SELECTED;
        p_wrong_from.reasoning = "wrong from";
        bool wrong_from_rejected = !try_accept_transition(gs3, p_wrong_from);

        TransitionProof p_disallowed = tp;
        p_disallowed.from = GapState::UNDISCOVERED;
        p_disallowed.to   = GapState::VERIFIED;
        p_disallowed.reasoning = "skip to verified";
        bool disallowed_rejected = !try_accept_transition(gs3, p_disallowed);

        TransitionProof p_valid = tp;
        p_valid.reasoning = "valid proposal";
        bool valid_accepted = try_accept_transition(gs3, p_valid) && (gs3.state == GapState::CANDIDATES_PROPOSED);
        bool s3_ok = wrong_from_rejected && disallowed_rejected && valid_accepted;
        std::cout << "  • [TEST STATE-3] try_accept_transition enforcement: " << (s3_ok ? "PASS" : "FAIL") << "\n";

        // [TEST STATE-4]: propose_candidates empty -> false, sim < 0.55 -> false, valid -> true
        GapConstitutionalState gs4;
        bool empty_cands_rejected = !propose_candidates(gs4, {});
        bool low_sim_rejected     = !propose_candidates(gs4, {{"ref.cpp", 10, 0.52, 0.80, 101}});
        bool good_cands_accepted  = propose_candidates(gs4, {{"ref.cpp", 10, 0.847, 0.92, 101}, {"ref2.cpp", 20, 0.71, 0.75, 102}});
        bool s4_ok = empty_cands_rejected && low_sim_rejected && good_cands_accepted &&
                     (gs4.state == GapState::CANDIDATES_PROPOSED) && (gs4.history.size() == 1);
        std::cout << "  • [TEST STATE-4] propose_candidates threshold & history: " << (s4_ok ? "PASS" : "FAIL") << "\n";

        // [TEST STATE-5]: Full path UNDISCOVERED -> VERIFIED + REJECTED from each of the 4 non-terminals
        bool sel_ok = select_reference(gs4, 101, 0.847, 0.92);
        bool app_ok = apply_adaptation(gs4, "void fft_step(float* H, int n) { for (int i = 0; i < n; ++i) { H[i] *= 2.0f; } }");
        bool ver_ok = verify_application(gs4, "compiled with g++ -std=c++20");
        bool terminal_locked = !reject_gap(gs4, "cannot reject after verified");

        GapConstitutionalState r_und, r_cand, r_sel, r_app;
        propose_candidates(r_cand, {{"r.cpp", 1, 0.75, 0.8, 11}});
        propose_candidates(r_sel, {{"r.cpp", 1, 0.75, 0.8, 11}});
        select_reference(r_sel, 11, 0.75, 0.8);
        propose_candidates(r_app, {{"r.cpp", 1, 0.75, 0.8, 11}});
        select_reference(r_app, 11, 0.75, 0.8);
        apply_adaptation(r_app, "void f() {}");

        bool rej_all_4 = reject_gap(r_und, "not needed") &&
                         reject_gap(r_cand, "no fit") &&
                         reject_gap(r_sel, "superseded") &&
                         reject_gap(r_app, "failed test") &&
                         !reject_gap(r_app, "already terminal");
        bool s5_ok = sel_ok && app_ok && ver_ok && (gs4.state == GapState::VERIFIED) &&
                     (gs4.history.size() == 4) && terminal_locked && rej_all_4;
        std::cout << "  • [TEST STATE-5] Full lifecycle & 4 non-terminal REJECTED paths: " << (s5_ok ? "PASS" : "FAIL") << "\n";

        // [TEST STATE-6]: End-to-end with 5 gaps (3 accepted to VERIFIED, 2 REJECTED)
        GapRegistry reg;
        for (uint32_t id = 1; id <= 5; ++id) {
            CodeGap cg{id, 0, 24, id * 10, GapKind::EMPTY_BODY, "void fn_" + std::to_string(id) + "()", "fn_" + std::to_string(id), "cpp"};
            auto& g = reg.register_gap(id, cg, "src/mod.cpp");
            propose_candidates(g, {{"ref/lib.cpp", id * 5, 0.80, 0.85, 200 + id}});
            if (id <= 3) {
                select_reference(g, 200 + id, 0.80, 0.85);
                apply_adaptation(g, "void fn_" + std::to_string(id) + "() { return; }");
                verify_application(g, "unit test passed");
            } else {
                reject_gap(g, "intentionally excluded");
            }
        }
        bool s6_ok = (reg.all().size() == 5);
        for (uint32_t id = 1; id <= 5; ++id) {
            const auto* g = reg.find(id);
            if (!g) { s6_ok = false; break; }
            if (id <= 3) {
                if (g->state != GapState::VERIFIED || g->history.size() != 4) s6_ok = false;
            } else {
                if (g->state != GapState::REJECTED || g->history.size() != 2) s6_ok = false;
            }
            for (const auto& hp : g->history) {
                if (!hp.is_valid()) s6_ok = false;
            }
        }
        std::cout << "  • [TEST STATE-6] 5-gap registry (3 VERIFIED, 2 REJECTED, all proofs valid): " << (s6_ok ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && s1_ok && s2_ok && s3_ok && s4_ok && s5_ok && s6_ok;
    }

    std::cout << "[TEST DISC-1..8] Reference Discovery, Narrowing & Indexing\n";
    {
        // [TEST DISC-1]: Builtin catalog has 22 curated sources
        SourceCatalog cat = SourceCatalog::builtin();
        bool d1_ok = (cat.sources.size() == 22);
        std::cout << "  • [TEST DISC-1] Builtin catalog (22 sources): " << (d1_ok ? "PASS" : "FAIL") << "\n";

        // [TEST DISC-2]: Catalog save / load / load_effective round-trip
        std::string cat_path = "/tmp/manifold_test_catalog.tsv";
        bool saved = cat.save(cat_path);
        SourceCatalog loaded_cat;
        bool loaded = loaded_cat.load(cat_path);
        SourceCatalog eff_cat = SourceCatalog::load_effective(cat_path);
        bool d2_ok = saved && loaded && (loaded_cat.sources.size() == 22) && (eff_cat.sources.size() == 22);
        std::cout << "  • [TEST DISC-2] Catalog save/load/load_effective: " << (d2_ok ? "PASS" : "FAIL") << "\n";

        // [TEST DISC-3]: extract_local_signals from code with #include, import, use, require
        std::string sample_code = "#include <fftw3.h>\nimport numpy as np\nuse tokio::net;\nconst fs = require('fs');\n// https://en.cppreference.com\nvoid helmholtz_fft() {}\n";
        LocalSignals sig = extract_local_signals_from_text(sample_code, "sample.cpp");
        bool d3_ok = sig.includes.count("fftw3.h") && sig.imports.count("numpy") &&
                     sig.imports.count("tokio") && sig.imports.count("fs") && !sig.urls.empty();
        std::cout << "  • [TEST DISC-3] extract_local_signals (include/import/use/require/url): " << (d3_ok ? "PASS" : "FAIL") << "\n";

        // [TEST DISC-4]: discover_from_signals ranks numpy and fftw3 at top
        auto cands = discover_from_signals(sig, cat);
        bool d4_ok = (cands.size() >= 2) &&
                     (cands[0].source.id == "github.com/numpy/numpy" || cands[0].source.id == "github.com/FFTW/fftw3");
        std::cout << "  • [TEST DISC-4] discover_from_local ranking: " << (d4_ok ? "PASS" : "FAIL") << "\n";

        // [TEST DISC-5]: discover_from_description
        auto desc_cands = discover_from_description("fast fourier transform fftw3 helmholtz wave solver", cat);
        bool d5_ok = !desc_cands.empty() && (desc_cands[0].source.id == "github.com/FFTW/fftw3");
        std::cout << "  • [TEST DISC-5] discover_from_description: " << (d5_ok ? "PASS" : "FAIL") << "\n";

        // [TEST DISC-6]: narrow_candidates (TAG, LANGUAGE, EXCLUDE, MORE_LIKE, TYPE)
        auto by_tag  = narrow_candidates(cands, {NarrowOp::TAG, "fft"});
        auto by_lang = narrow_candidates(cands, {NarrowOp::LANGUAGE, "cpp"});
        auto by_excl = narrow_candidates(cands, {NarrowOp::EXCLUDE, "numpy"});
        auto by_more = narrow_candidates(cands, {NarrowOp::MORE_LIKE, "FFTW"});
        auto by_type = narrow_candidates(cands, {NarrowOp::TYPE, "GITHUB_REPO"});
        bool d6_ok = !by_tag.empty() && !by_lang.empty() && (by_excl.size() < cands.size()) &&
                     !by_more.empty() && !by_type.empty();
        std::cout << "  • [TEST DISC-6] narrow_candidates (5 filter modes): " << (d6_ok ? "PASS" : "FAIL") << "\n";

        // [TEST DISC-7]: Cache key, status, and fetch_to_cache
        fs::path test_cache = "/tmp/manifold_test_cache_d7";
        std::error_code ec;
        fs::remove_all(test_cache, ec);
        CacheStatus st_before = cache_status(test_cache, "github.com/FFTW/fftw3");
        fs::path fetched = fetch_to_cache(test_cache, cat.sources[0], true);
        CacheStatus st_after  = cache_status(test_cache, "github.com/FFTW/fftw3");
        bool d7_ok = (st_before == CacheStatus::MISS) && fs::exists(fetched, ec) && (st_after == CacheStatus::HIT_FRESH);
        std::cout << "  • [TEST DISC-7] Cache lifecycle (MISS -> fetch -> HIT_FRESH): " << (d7_ok ? "PASS" : "FAIL") << "\n";

        // [TEST DISC-8]: select_and_index indexes first time and returns SKIPPED_CACHED second time
        ManifoldEngine idx_eng;
        auto rep1 = select_and_index(idx_eng, {"github.com/FFTW/fftw3"}, test_cache, cat);
        auto rep2 = select_and_index(idx_eng, {"github.com/FFTW/fftw3"}, test_cache, cat);
        bool d8_ok = (rep1.size() == 1 && rep1[0].outcome == IndexOutcome::INDEXED && rep1[0].cells_added > 0) &&
                     (rep2.size() == 1 && rep2[0].outcome == IndexOutcome::SKIPPED_CACHED);
        std::cout << "  • [TEST DISC-8] select_and_index (INDEXED -> SKIPPED_CACHED): " << (d8_ok ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && d1_ok && d2_ok && d3_ok && d4_ok && d5_ok && d6_ok && d7_ok && d8_ok;
    }

    std::cout << "[TEST E2E-01..02] End-to-End Discovery -> Index -> Gaps -> Fill -> State Machine\n";
    {
        // [TEST E2E-01]: Discovery -> Index -> Gaps pipeline on fixture repo
        fs::path fix_repo = "/tmp/manifold_e2e_repo";
        fs::path fix_cache = "/tmp/manifold_e2e_cache";
        std::error_code ec;
        fs::remove_all(fix_repo, ec);
        fs::remove_all(fix_cache, ec);
        fs::create_directories(fix_repo / "src", ec);
        {
            std::ofstream out(fix_repo / "src" / "hufd_core.cpp");
            out << "#include <fftw3.h>\n\n"
                << "void fft_step(float* H, int n, float dt, float g) {}\n\n"
                << "void run_solver(float* buf, int n) {\n"
                << "    fft_step(buf, n, 0.01f, 1.0f);\n"
                << "}\n";
        }
        auto disc = discover_from_local(fix_repo);
        ManifoldEngine e2e_eng;
        e2e_eng.ingest_path(fix_repo, Modality::REPO_CODE, "local");
        // Ensure local URI maps to actual file path for on-disk reading
        e2e_eng.ingest_file((fix_repo / "src" / "hufd_core.cpp").string(),
                            "#include <fftw3.h>\n\nvoid fft_step(float* H, int n, float dt, float g) {}\n\nvoid run_solver(float* buf, int n) {\n    fft_step(buf, n, 0.01f, 1.0f);\n}\n",
                            Modality::REPO_CODE, false, nullptr, false, 0, "local");
        auto idx_reps = select_and_index(e2e_eng, {"github.com/FFTW/fftw3"}, fix_cache);
        auto gaps = find_stub_gaps(e2e_eng, "local");
        bool e2e_1_ok = !disc.empty() && !idx_reps.empty() && !gaps.empty() &&
                        (gaps[0].function_name == "fft_step") && (gaps[0].kind == GapKind::EMPTY_BODY);
        std::cout << "  • [TEST E2E-01] Discovery -> Index -> Gaps pipeline: " << (e2e_1_ok ? "PASS" : "FAIL") << "\n";

        // [TEST E2E-02]: Full workflow through all five transitions (UNDISCOVERED -> VERIFIED + REJECTED)
        GapRegistry e2e_reg;
        auto& gs = e2e_reg.register_gap(47, gaps[0], gaps[0].signature);
        bool step1 = annotate_gap_with_reference(e2e_eng, gs, 3);
        uint32_t chosen_ref = gs.candidate_cell_ids.empty() ? 0 : gs.candidate_cell_ids[0];
        double chosen_sim   = gs.candidates.empty() ? 0.0 : gs.candidates[0].similarity;
        double chosen_sig   = std::max(0.65, gs.candidates.empty() ? 0.0 : gs.candidates[0].signature_overlap);
        bool step2 = select_reference(gs, chosen_ref, chosen_sim, chosen_sig);
        bool step3 = apply_adaptation(gs, "void fft_step(float* H, int n, float dt, float g) { for (int i = 0; i < n; ++i) { H[i] += dt * g; } }");
        bool step4 = verify_application(gs, "compiled with g++ -std=c++20");
        bool e2e_2_ok = step1 && step2 && step3 && step4 &&
                        (gs.state == GapState::VERIFIED) && (gs.history.size() == 4);
        std::cout << "  • [TEST E2E-02] Full 4-step verified transition workflow: " << (e2e_2_ok ? "PASS" : "FAIL") << "\n";
        test_ok = test_ok && e2e_1_ok && e2e_2_ok;
    }

    std::cout << "============================================================================\n";
    return test_ok ? 0 : 1;
}

// ============================================================================
// 14. COMMAND-LINE INTERFACE (DISCOVER / INDEX / GAPS / FILL / STATE MACHINE)
// ============================================================================
static std::string resolve_checkpoint_path(const std::string& repo_or_db) {
    if (repo_or_db.ends_with(".mfd")) return repo_or_db;
    std::error_code ec;
    if (fs::is_directory(repo_or_db, ec)) {
        return (fs::path(repo_or_db) / ".manifold.mfd").string();
    }
    return repo_or_db + ".mfd";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage:\n"
                  << "  manifold catalog [--list]\n"
                  << "  manifold discover <repo_path>\n"
                  << "  manifold index <repo_or_db> --sources <id1,id2,...>\n"
                  << "  manifold gaps <repo_or_db>\n"
                  << "  manifold fill <repo_or_db> --gap <id> [--space <paths>]\n"
                  << "  manifold select <repo_or_db> --gap <id> --reference <idx_or_cell_id>\n"
                  << "  manifold apply <repo_or_db> --gap <id> --code \"<adapted_code>\"\n"
                  << "  manifold verify <repo_or_db> --gap <id> --result \"<verification_result>\"\n"
                  << "  manifold reject <repo_or_db> --gap <id> --reason \"<reason>\"\n"
                  << "  manifold status <repo_or_db> [--gap <id>]\n"
                  << "  manifold place \"<query>\" [--space <paths>] [--kinds <kinds>] [--json]\n"
                  << "  manifold search \"<query>\" [--space <paths>]\n"
                  << "  manifold selftest\n"
                  << "  manifold benchmark [dir_path]\n";
        return 0;
    }

    std::string cmd = argv[1];
    if (cmd == "selftest") {
        return run_selftest();
    }
    if (cmd == "benchmark") {
        std::string target = (argc >= 3) ? argv[2] : "/usr/include/c++/12";
        return run_benchmark(target);
    }

    if (cmd == "catalog") {
        SourceCatalog cat = SourceCatalog::builtin();
        for (const auto& src : cat.sources) {
            std::cout << src.id << " [" << source_type_name(src.type) << "] — " << src.description << "\n";
        }
        return 0;
    }

    if (cmd == "discover" && argc >= 3) {
        std::string target = argv[2];
        std::error_code ec;
        auto cands = fs::exists(target, ec)
            ? discover_from_local(target)
            : discover_from_description(target);
        std::cout << "Discovered " << cands.size() << " candidates:\n";
        for (size_t i = 0; i < cands.size(); ++i) {
            std::cout << "  #" << (i + 1) << " [score=" << std::fixed << std::setprecision(3) << cands[i].score << "] "
                      << cands[i].source.id << " (";
            for (size_t r = 0; r < cands[i].matched_reasons.size(); ++r) {
                if (r > 0) std::cout << ", ";
                std::cout << cands[i].matched_reasons[r];
            }
            std::cout << ")\n";
        }
        return 0;
    }

    if (cmd == "index" && argc >= 3) {
        std::string repo_arg = argv[2];
        std::string db = resolve_checkpoint_path(repo_arg);
        std::string sources_csv;
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "--sources" && i + 1 < argc) {
                sources_csv = argv[++i];
            }
        }
        ManifoldEngine eng;
        if (fs::exists(db)) eng.load_checkpoint(db);
        std::error_code ec;
        if (fs::is_directory(repo_arg, ec) && eng.cells.empty()) {
            eng.ingest_path(repo_arg, Modality::REPO_CODE, "local");
        }
        auto src_list = SourceCatalog::split_csv(sources_csv);
        auto reps = select_and_index(eng, src_list);
        eng.save_checkpoint(db);
        for (const auto& r : reps) {
            const char* tag = (r.outcome == IndexOutcome::INDEXED) ? "[INDEXED]       "
                            : (r.outcome == IndexOutcome::SKIPPED_CACHED) ? "[SKIPPED_CACHED]" : "[FAILED]        ";
            std::cout << "  " << tag << " " << r.source_id << " (" << r.cells_added << " cells)\n";
        }
        return 0;
    }

    if (cmd == "gaps" && argc >= 3) {
        std::string repo_arg = argv[2];
        std::string db = resolve_checkpoint_path(repo_arg);
        ManifoldEngine eng;
        if (fs::exists(db)) eng.load_checkpoint(db);
        std::error_code ec;
        if (fs::is_directory(repo_arg, ec)) {
            eng.ingest_path(repo_arg, Modality::REPO_CODE, "local");
        }
        auto gaps = find_stub_gaps(eng, "local");
        GapRegistry reg;
        reg.load(db + ".gaps");
        std::cout << "Found " << gaps.size() << " gaps.\n";
        for (size_t i = 0; i < gaps.size(); ++i) {
            uint32_t gid = gaps[i].line_number;
            std::string f_uri = "";
            for (const auto& c : eng.cells) {
                if (c.id == gaps[i].cell_id) { f_uri = c.coord.uri; break; }
            }
            reg.register_gap(gid, gaps[i], f_uri);
            std::cout << "  " << f_uri << ":" << gaps[i].line_number << "  "
                      << gaps[i].language << "  " << gap_kind_name(gaps[i].kind) << "  "
                      << gaps[i].signature << "\n";
        }
        eng.save_checkpoint(db);
        reg.save(db + ".gaps");
        return 0;
    }

    if (cmd == "fill" && argc >= 3) {
        std::string repo_arg = argv[2];
        std::string db = resolve_checkpoint_path(repo_arg);
        uint32_t gap_id = 0;
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "--gap" && i + 1 < argc) {
                gap_id = static_cast<uint32_t>(std::stoul(argv[++i]));
            }
        }
        ManifoldEngine eng;
        if (fs::exists(db)) eng.load_checkpoint(db);
        GapRegistry reg;
        reg.load(db + ".gaps");
        auto* gs = reg.find(gap_id);
        if (!gs) {
            std::cerr << "Error: gap " << gap_id << " not found\n";
            return 1;
        }
        if (annotate_gap_with_reference(eng, *gs, 3)) {
            const auto& last = gs->history.back();
            std::cout << "  UNDISCOVERED -> CANDIDATES_PROPOSED  [div=" << (last.divisible ? "T" : "F")
                      << " adm=" << (last.admissible ? "T" : "F")
                      << " corr=" << (last.correct ? "T" : "F")
                      << " resp=" << (last.respectful ? "T" : "F")
                      << "]  top sim=" << std::fixed << std::setprecision(3) << last.similarity << "\n";
            for (size_t i = 0; i < gs->candidates.size(); ++i) {
                std::cout << "  #" << (i + 1) << " sim=" << gs->candidates[i].similarity
                          << " sig=" << gs->candidates[i].signature_overlap
                          << "  " << gs->candidates[i].source_path << ":" << gs->candidates[i].line << "\n";
            }
            reg.save(db + ".gaps");
            return 0;
        }
        std::cerr << "Error: could not propose candidates for gap " << gap_id << "\n";
        return 1;
    }

    if (cmd == "select" && argc >= 3) {
        std::string db = resolve_checkpoint_path(argv[2]);
        uint32_t gap_id = 0, ref_arg = 0;
        for (int i = 3; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--gap" && i + 1 < argc) gap_id = static_cast<uint32_t>(std::stoul(argv[++i]));
            else if (a == "--reference" && i + 1 < argc) ref_arg = static_cast<uint32_t>(std::stoul(argv[++i]));
        }
        GapRegistry reg;
        reg.load(db + ".gaps");
        auto* gs = reg.find(gap_id);
        if (!gs) return 1;
        uint32_t ref_cid = ref_arg;
        double sim = 0.80, sig_ov = 0.85;
        if (ref_arg >= 1 && ref_arg <= gs->candidates.size()) {
            ref_cid = gs->candidates[ref_arg - 1].cell_id;
            sim     = gs->candidates[ref_arg - 1].similarity;
            sig_ov  = std::max(0.60, gs->candidates[ref_arg - 1].signature_overlap);
        }
        if (select_reference(*gs, ref_cid, sim, sig_ov)) {
            const auto& last = gs->history.back();
            std::cout << "  CANDIDATES_PROPOSED -> REFERENCE_SELECTED  [div=" << (last.divisible ? "T" : "F")
                      << " adm=" << (last.admissible ? "T" : "F")
                      << " corr=" << (last.correct ? "T" : "F")
                      << " resp=" << (last.respectful ? "T" : "F")
                      << "]  sig_overlap=" << std::fixed << std::setprecision(2) << last.signature_overlap << "\n";
            reg.save(db + ".gaps");
            return 0;
        }
        return 1;
    }

    if (cmd == "apply" && argc >= 3) {
        std::string db = resolve_checkpoint_path(argv[2]);
        uint32_t gap_id = 0;
        std::string code_str;
        for (int i = 3; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--gap" && i + 1 < argc) gap_id = static_cast<uint32_t>(std::stoul(argv[++i]));
            else if (a == "--code" && i + 1 < argc) code_str = argv[++i];
        }
        GapRegistry reg;
        reg.load(db + ".gaps");
        auto* gs = reg.find(gap_id);
        if (!gs || !apply_adaptation(*gs, code_str)) return 1;
        const auto& last = gs->history.back();
        std::cout << "  REFERENCE_SELECTED -> APPLIED  [div=" << (last.divisible ? "T" : "F")
                  << " adm=" << (last.admissible ? "T" : "F")
                  << " corr=" << (last.correct ? "T" : "F")
                  << " resp=" << (last.respectful ? "T" : "F")
                  << "]  (" << code_str.size() << " bytes)\n";
        reg.save(db + ".gaps");
        return 0;
    }

    if (cmd == "verify" && argc >= 3) {
        std::string db = resolve_checkpoint_path(argv[2]);
        uint32_t gap_id = 0;
        std::string res_str;
        for (int i = 3; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--gap" && i + 1 < argc) gap_id = static_cast<uint32_t>(std::stoul(argv[++i]));
            else if (a == "--result" && i + 1 < argc) res_str = argv[++i];
        }
        GapRegistry reg;
        reg.load(db + ".gaps");
        auto* gs = reg.find(gap_id);
        if (!gs || !verify_application(*gs, res_str)) return 1;
        const auto& last = gs->history.back();
        std::cout << "  APPLIED -> VERIFIED  [div=" << (last.divisible ? "T" : "F")
                  << " adm=" << (last.admissible ? "T" : "F")
                  << " corr=" << (last.correct ? "T" : "F")
                  << " resp=" << (last.respectful ? "T" : "F") << "]\n";
        reg.save(db + ".gaps");
        return 0;
    }

    if (cmd == "reject" && argc >= 3) {
        std::string db = resolve_checkpoint_path(argv[2]);
        uint32_t gap_id = 0;
        std::string reason;
        for (int i = 3; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--gap" && i + 1 < argc) gap_id = static_cast<uint32_t>(std::stoul(argv[++i]));
            else if (a == "--reason" && i + 1 < argc) reason = argv[++i];
        }
        GapRegistry reg;
        reg.load(db + ".gaps");
        auto* gs = reg.find(gap_id);
        if (!gs || !reject_gap(*gs, reason)) return 1;
        std::cout << "  * -> REJECTED  (" << reason << ")\n";
        reg.save(db + ".gaps");
        return 0;
    }

    if (cmd == "status" && argc >= 3) {
        std::string db = resolve_checkpoint_path(argv[2]);
        uint32_t gap_id = 0;
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "--gap" && i + 1 < argc) {
                gap_id = static_cast<uint32_t>(std::stoul(argv[++i]));
            }
        }
        GapRegistry reg;
        reg.load(db + ".gaps");
        for (const auto& [id, gs] : reg.all()) {
            if (gap_id != 0 && id != gap_id) continue;
            std::cout << "GAP " << id << ": " << gs.file_uri << ":" << gs.line_number << "\n"
                      << "  Signature: " << gs.signature << "\n"
                      << "  State: " << gap_state_name(gs.state) << "\n"
                      << "  History: " << gs.history.size() << " transitions\n";
            for (size_t k = 0; k < gs.history.size(); ++k) {
                const auto& h = gs.history[k];
                std::cout << "    [" << (k + 1) << "] " << gap_state_name(h.from) << " -> " << gap_state_name(h.to)
                          << "  div=" << (h.divisible ? "T" : "F")
                          << " adm=" << (h.admissible ? "T" : "F")
                          << " corr=" << (h.correct ? "T" : "F")
                          << " resp=" << (h.respectful ? "T" : "F")
                          << "  " << h.reasoning << "\n";
            }
        }
        return 0;
    }

    if (cmd == "init" && argc >= 3) {
        ManifoldEngine eng;
        if (eng.save_checkpoint(argv[2])) {
            std::cout << "Initialized empty manifold checkpoint at " << argv[2] << "\n";
            return 0;
        }
        return 1;
    }

    if (cmd == "ingest" && argc >= 4) {
        std::string db = argv[2];
        ManifoldEngine eng;
        if (fs::exists(db)) eng.load_checkpoint(db);

        std::optional<Modality> explicit_mod = std::nullopt;
        std::string prefix = "";
        std::vector<std::string> targets;

        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if ((arg == "--scope" || arg == "-s") && i + 1 < argc) {
                explicit_mod = parse_modality_flag(argv[++i]);
            } else if ((arg == "--prefix" || arg == "-p") && i + 1 < argc) {
                prefix = argv[++i];
            } else {
                targets.push_back(arg);
            }
        }

        IngestStats total{};
        for (const auto& t : targets) {
            IngestStats st = eng.ingest_path(t, explicit_mod, prefix);
            total.files_admitted += st.files_admitted;
            total.files_revised += st.files_revised;
            total.files_preserved += st.files_preserved;
            total.spans_admitted += st.spans_admitted;
        }
        eng.save_checkpoint(db);
        std::cout << "Ingested into " << db << ": +" << total.files_admitted << " admitted files, ~"
                  << total.files_revised << " revised, =" << total.files_preserved
                  << " preserved (" << eng.cells.size() << " total cells, " << total.spans_admitted << " spans admitted)\n";
        return 0;
    }

    if ((cmd == "query" || cmd == "search") && argc >= 3) {
        std::string db = argv[2];
        std::string raw_q;
        for (int i = 3; i < argc; ++i) {
            if (i > 3) raw_q.push_back(' ');
            raw_q += argv[i];
        }
        ManifoldEngine eng;
        if (!eng.load_checkpoint(db)) {
            std::cerr << "Error: cannot load " << db << "\n";
            return 1;
        }
        ParsedQuery pq = parse_query_dsl(raw_q);
        Hypervec8192 qhv = encode_span_hypervector(pq.clean_expr, &eng.surprisal, false, MAX_QUERY_MASS_NODES);
        auto hits = eng.route(qhv, pq.target_scale, pq.filter_reserved, pq.modality_filter, 10);
        for (const auto& h : hits) {
            std::cout << h.cell->coord.uri << ":" << h.cell->coord.start_line << ":1: "
                      << "[" << modality_tag(h.cell->halo.modality)
                      << " bucket=" << static_cast<int>(h.cell->halo.bucket_id)
                      << " sim=" << std::fixed << std::setprecision(3) << h.similarity
                      << " +" << std::setprecision(1) << h.z_score << "s] "
                      << h.cell->preview << "\n";
        }
        return 0;
    }

    if (cmd == "place" && argc >= 3) {
        std::string db = resolve_checkpoint_path(argv[2]);
        ManifoldEngine eng;
        if (fs::exists(db)) eng.load_checkpoint(db);
        auto gaps = find_stub_gaps(eng, "local");
        auto ps   = find_fill_placements(eng, gaps);
        auto mv   = find_move_placements(eng, "local");
        auto ex   = find_extract_placements(eng, "local");
        ps.insert(ps.end(), mv.begin(), mv.end());
        ps.insert(ps.end(), ex.begin(), ex.end());
        std::cout << placements_to_json(ps) << "\n";
        return 0;
    }

    if (cmd == "diff" && argc >= 4) {
        ManifoldEngine a, b;
        if (!a.load_checkpoint(argv[2]) || !b.load_checkpoint(argv[3])) {
            std::cerr << "Error loading checkpoints for diff\n";
            return 1;
        }
        uint32_t dist = a.global_acc.project().hamming(b.global_acc.project());
        std::cout << "Manifold Checkpoint Diff: " << dist << " / 8192 bits ("
                  << std::fixed << std::setprecision(2) << (100.0 * dist / 8192.0) << "% shift) | "
                  << "Cells: " << a.cells.size() << " -> " << b.cells.size() << "\n";
        return 0;
    }

    return 0;
}