#include "../common/InvertedIndex.h"
#include "../common/Iterator.h"
#include "../common/SkipTree.h"
#include "../common/utils.h"

#include <cstdint>
#include <ctype.h>

#include <vector>

using namespace cpot;

struct NlpIndex {
  struct NlpRow {
    uint64_t docid : 48;
    uint16_t loc;
    friend bool operator==(NlpRow a, NlpRow b) {
      return a.docid == b.docid && a.loc == b.loc;
    }
    friend bool operator<=(NlpRow a, NlpRow b) {
      if (a.docid == b.docid) {
        return a.loc <= b.loc;
      }
      return a.docid <= b.docid;
    }
    friend bool operator<(NlpRow a, NlpRow b) {
      if (a.docid == b.docid) {
        return a.loc < b.loc;
      }
      return a.docid < b.docid;
    }
    friend std::ostream& operator<<(std::ostream& s, NlpRow row) {
      return s << "[" << row.docid << "," << row.loc << "]";
    }
    static NlpRow smallest() {
      return NlpRow{uint64_t(0), uint16_t(0)};
    }
    static NlpRow largest() {
      return NlpRow{uint64_t(281474976710655), uint16_t(-1)};
    }
  };
  struct TokenMatch {
    NlpRow row;
    uint64_t token;
    friend bool operator<(TokenMatch a, TokenMatch b) {
      if (a.row == b.row) {
        return a.token < b.token;
      }
      return a.row < b.row;
    }
    friend std::ostream& operator<<(std::ostream& s, TokenMatch tm) {
      return s << "[" << tm.row << "," << tm.token << "]";
    }
  };

  static void lower(std::string *text) {
    for (size_t i = 0; i < text->size(); ++i) {
      if ((*text)[i] >= 'A' && (*text)[i] <= 'Z') {
        (*text)[i] += 'a' - 'A';
      }
    }
  }

  // Strips string, removes symbols, removes repeated whitespace.
  static std::vector<std::string> str2words(std::string text) {
    NlpIndex::lower(&text);

    std::vector<std::string> tokens;

    std::string accumulator;
    for (char c : text) {
      if (isalnum(c)) {
        accumulator += c;
        continue;
      }
      if (c == '\'') {
        continue;
      }
      if (accumulator.size() > 0) {
        tokens.push_back(accumulator);
        accumulator = "";
      }
      if (c == ' ' || c == '.' || c == '!' || c == '?' || c == ';' || c == ',') {
        continue;
      }
      tokens.push_back(std::string() + c);
    }
    if (accumulator.size() > 0) {
      tokens.push_back(accumulator);
    }

    return tokens;
  }

  static uint64_t hash(const std::string& word) {
    assert(word.size() <= 8);
    uint64_t h = 0;
    for (const auto & c : word) {
      h <<= 8;
      h |= uint64_t(c);
    }
    return h;
  }

  static void word2ints(const std::string& word, std::vector<uint64_t>* results) {
    if (word.size() <= 8) {
      results->push_back(NlpIndex::hash(word));
      return;
    }
    // Grab first 7 and last 7 characters from word.
    results->push_back(NlpIndex::hash(word.substr(0, 7) + "."));
    results->push_back(NlpIndex::hash("." + word.substr(word.size() - 7, word.size())));
  }

  static std::vector<uint64_t> str2tokens(std::string text) {
    std::vector<uint64_t> tokens;
    for (const auto& word : NlpIndex::str2words(text)) {
      NlpIndex::word2ints(word, &tokens);
    }
    return tokens;
  }

  NlpIndex(std::string filename) : index_(filename) {}
  InvertedIndex<NlpRow> index_;

  void add_document(uint64_t docid, const std::string& text) {
    std::vector<uint64_t> tokens = NlpIndex::str2tokens(text);
    for (size_t i = 0; i < tokens.size(); ++i) {
      index_.insert(tokens[i], NlpRow{docid, uint16_t(i)});
    }
  }

  std::vector<uint64_t> documents_with(const std::string& text, uint64_t low = 0, uint64_t high = uint64_t(-1)) {
    std::vector<uint64_t> tokens;
    for (const auto& word : NlpIndex::str2words(text)) {
      NlpIndex::word2ints(word, &tokens);
    }

    std::vector<TokenMatch> matches;
    for (const auto& token : tokens) {
      std::vector<NlpRow> M = index_.range(token, NlpRow{low, 0}, NlpRow{high, uint16_t(-1)});
      for (const auto& m : M) {
        matches.push_back(TokenMatch{m, token});
      }
    }

    std::sort(matches.begin(), matches.end());

    std::vector<uint64_t> results;

    auto it = matches.begin();
    while (it < matches.end()) {
      auto it2 = it;
      while (it2 < matches.end() && it2->row.docid == it->row.docid) {
        it2++;
      }

      std::vector<TokenMatch> matchesInDoc(it, it2);
      for (size_t i = 0; i < matchesInDoc.size() - tokens.size() + 1; ++i) {
        bool badMatch = false;
        if (matchesInDoc[i].token != tokens[0]) {
          continue;
        }
        for (size_t j = 1; j < tokens.size(); ++j) {
          if (matchesInDoc[i + j].token != tokens[j] || matchesInDoc[i + j].row.loc != matchesInDoc[i].row.loc + j) {
            badMatch = true;
            break;
          }
        }
        if (!badMatch) {
          results.push_back(it->row.docid);
          break;
        }
      }

      it = it2;
    }

    return results;
  }
};