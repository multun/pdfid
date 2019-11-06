#pragma once

#include <ostream>
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <gmpxx.h>

// www.cs.uvic.ca/~ruskey/Publications/RankPerm/RankPerm.html

template<class Alpha>
static void unrank1(std::vector<Alpha> &v, mpz_class r)
{
    std::unordered_map<size_t, Alpha> trans_map;
    std::vector<size_t> int_vect;
    for (size_t i = 0; i < v.size(); i++)
    {
        trans_map[i] = v[i];
        int_vect.push_back(i);
    }

    unrank1(int_vect, r);
    for (size_t i = 0; i < v.size(); i++)
        v[i] = trans_map[int_vect[i]];
}

template<>
void unrank1(std::vector<size_t> &v, mpz_class r)
{
    for (size_t n = v.size(); n; n--)
    {
        std::swap(v[n-1], v[mpz_class(r % n).get_ui()]);
        r /= n;
    }
}

static mpz_class rank1(size_t n, std::vector<size_t> &v, std::vector<size_t> &v_i)
{
    if (n == 1)
        return 0;

    size_t s = v[n - 1];
    std::swap(v[n - 1], v[v_i[n - 1]]);
    std::swap(v_i[s], v_i[n - 1]);
    return s + n * rank1(n - 1, v, v_i);
}

static mpz_class rank1(std::vector<size_t> &v)
{
    std::vector<size_t> v_i(v.size());
    for (size_t i = 0; i < v.size(); i++)
        v_i[v[i]] = i;
    return rank1(v.size(), v, v_i);
}


template<class Alpha>
static mpz_class rank1(std::vector<Alpha> &id, std::vector<Alpha> &v)
{
    std::unordered_map<Alpha, size_t> trans_map;
    for (size_t i = 0; i < v.size(); i++)
        trans_map[id[i]] = i;

    std::vector<size_t> int_vect;
    for (size_t i = 0; i < v.size(); i++)
        int_vect.push_back(trans_map[v[i]]);
    return rank1(int_vect);
}

using map_t = std::map<std::string, int>;

static bool comp_dict_vals(const map_t::value_type *i, const map_t::value_type *j) {
    const std::string &i_key = i->first;
    const std::string &j_key = j->first;
    return i_key < j_key;
}

static mpz_class mpz_class_fac(size_t n)
{
    mpz_class res;
    mpz_fac_ui(res.get_mpz_t(), n);
    return res;
}

struct bit_istream
{
    bit_istream(std::istream &stream)
        : stream(stream)
        , data(0)
        , cur_bit(8)
    {}

    int next()
    {
        if (data == -1)
            return -1;

        if (cur_bit == 8)
        {
            if (stream.eof())
            {
                data = -1;
                return -1;
            }
            data = stream.get();
            cur_bit = 0;
        }

        return (data >> cur_bit++) & 1;
    }

private:
    size_t cur_bit;
    int data;
    std::istream &stream;
};

struct bit_ostream
{
    bit_ostream(std::ostream &stream)
        : stream(stream)
        , data(0)
        , cur_bit(0)
    {}

    void push_bit(bool bit)
    {
        data |= (uint8_t)bit << cur_bit++;
        if (cur_bit == 8)
        {
            cur_bit = 0;
            stream.put(data);
            data = 0;
        }
    }

    void push(long offset, mpz_class &&number, size_t size)
    {
        state_map.emplace(offset, std::make_pair(number, size));
    }

    void flush()
    {
        for (auto &item: state_map)
        {
            auto &value = item.second;
            auto &number = value.first;
            auto size = value.second;
            for (size_t i = 0; i < size; i++)
                push_bit(mpz_tstbit(number.get_mpz_t(), i));
        }
    }

private:
    // as the file isn't read linearly, data is inserted in a global map
    // and gets reconstructed at the end.
    std::map<long, std::pair<mpz_class, size_t>> state_map;
    size_t cur_bit;
    uint8_t data;
    std::ostream &stream;
};

static size_t mpz_class_sizeinbase(const mpz_class& num, size_t base)
{
    return mpz_sizeinbase(num.get_mpz_t(), base);
}

static size_t perm_available_bits(const mpz_class &permutation_count)
{
    // simple tests here:
    // assert(perm_available_bits(6) == 2);
    // assert(perm_available_bits(7) == 2);
    // assert(perm_available_bits(8) == 3);
    // assert(perm_available_bits(9) == 3);
    // assert(perm_available_bits(10) == 3);
    return mpz_class_sizeinbase(permutation_count, 2) - 1;
}

static size_t size_available_bits(size_t size)
{
    // count permutations
    auto permutation_count = mpz_class_fac(size);

    // count how many bits can be stored for the given permutation count
    return perm_available_bits(permutation_count);
}

static std::vector<const map_t::value_type*> map_reference(map_t map)
{
    std::vector<const map_t::value_type*> map_vect;
    for(const auto& item : map)
        map_vect.push_back(&item);
    auto sorted_map_vect = map_vect;
    std::sort(sorted_map_vect.begin(), sorted_map_vect.end(), comp_dict_vals);
    return sorted_map_vect;
}

static mpz_class bit_istream_pull_mpz(bit_istream &bit_istream, size_t available_bits)
{
    mpz_class permutation_id = 0;
    for (size_t i = 0; i < available_bits; i++)
    {
        int cur_bit = bit_istream.next();
        if (cur_bit == -1)
            break;

        if (cur_bit)
            mpz_setbit(permutation_id.get_mpz_t(), i);
    }
    return permutation_id;
}
