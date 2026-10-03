#include "aiws/processing_core.hpp"

#include "aiws/chunker.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/corpus_index.hpp"
#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <cmath>

namespace aiws
{

    struct ProcessingCore::Impl
    {

    public:
        std::vector<Chunk> chunks;
        CorpusIndex index;

        // M2 TODO: refactor the processing components so ProcessingCore owns and
        // uses the supplied strategy objects polymorphically. The concrete M1
        // members below keep the starter's default path runnable.
        std::unique_ptr<ChunkingStrategy> chunker;
        std::unique_ptr<RetrievalStrategy> retrieval;
        std::unique_ptr<ContextStrategy> context;

        // Create the default struct
        Impl() : chunker(std::make_unique<Chunker>(ChunkingPolicy{kMaxChunkTokens, kChunkOverlap, kParagraphPreferenceWindow})),
                 retrieval(std::make_unique<RetrievalEngine>()), context(std::make_unique<ContextBuilder>()) {}

        // Set chunker/retrieval/context for three arg constructor
        Impl(std::unique_ptr<ChunkingStrategy> chunking, std::unique_ptr<RetrievalStrategy> retrieval_strategy, std::unique_ptr<ContextStrategy> context_strategy)
            : chunker(std::move(chunking)), retrieval(std::move(retrieval_strategy)), context(std::move(context_strategy)) {}
    };

    ProcessingCore::ProcessingCore() : impl_(std::make_unique<Impl>()) {}

    ProcessingCore::ProcessingCore(std::unique_ptr<ChunkingStrategy> chunking,
                                   std::unique_ptr<RetrievalStrategy> retrieval,
                                   std::unique_ptr<ContextStrategy> context)
    {
        // M2 TODO: validate non-null strategies, take exclusive ownership, and
        // compose the processing core from them.
        if (chunking == nullptr || retrieval == nullptr || context == nullptr)
        {
            throw std::invalid_argument("Invalid Strategies: Cannot be a Null Pointer");
        }

        impl_ = std::make_unique<Impl>(std::move(chunking), std::move(retrieval), std::move(context));
    }

    ProcessingCore::~ProcessingCore() = default;
    ProcessingCore::ProcessingCore(ProcessingCore &&) noexcept = default;
    ProcessingCore &ProcessingCore::operator=(ProcessingCore &&) noexcept = default;

    std::string ProcessingCore::normalize(const std::string &text)
    {
        // TODO: return the normalized form of the input text.
        return TextProcessor::normalize(text);
    }

    void ProcessingCore::rebuild(const Workspace &workspace)
    {
        // TODO: rebuild the processing state from the workspace.

        // Stores the ID's we have already seen
        std::vector<std::string> documentIds;

        // First loop validates document ID's
        for (const Document &doc : workspace.documents())
        {
            // Checks new ID against existing list
            for (std::string id : documentIds)
            {
                if (doc.id() == id)
                {
                    throw std::invalid_argument("Repeated Document ID's - Corpus remains unchanged");
                }
            }

            documentIds.push_back(doc.id());
        }

        Chunker chunker;
        std::vector<Chunk> newChunks;
        std::size_t docOrder = 0;

        for (const Document &doc : workspace.documents())
        {
            std::vector<Chunk> tempChunks = chunker.chunk(doc, docOrder);
            newChunks.insert(newChunks.end(), tempChunks.begin(), tempChunks.end());
            docOrder++;
        }

        CorpusIndex newIndex;
        newIndex.build(newChunks);

        impl_->chunks = std::move(newChunks);
        impl_->index = std::move(newIndex);
    }

    const std::vector<Chunk> &ProcessingCore::chunks() const noexcept
    {
        // TODO: return the chunks currently stored by the processing core.
        return impl_->chunks;
    }

    std::size_t ProcessingCore::chunk_count() const noexcept
    {
        // TODO: return the number of stored chunks.
        return impl_->chunks.size();
    }

    std::size_t ProcessingCore::document_frequency(const std::string &term) const
    {
        // TODO: return the document frequency for the requested term.
        return impl_->index.document_frequency(term);
    }

    std::size_t ProcessingCore::term_frequency(const std::string &term,
                                               const std::string &chunk_id) const
    {
        // TODO: return the term frequency for the requested chunk.
        return impl_->index.term_frequency(term, chunk_id);
    }

    /*
    SearchResult elements:

    std::string chunk_id;
    std::string document_id;
    std::size_t chunk_sequence{};
    std::string text;
    double score{};
    std::size_t matched_terms{};
    */

    std::vector<SearchResult> ProcessingCore::search(const std::string &query, int k) const
    {
        // TODO: return the ranked results for the requested query.

        if (k < 0)
        {
            throw std::invalid_argument("Invalid Input: k is negative");
        }
        else if (k == 0)
        {
            return {};
        }

        std::vector<std::string> queryTerms = normalizeQuery(query);

        if (queryTerms.empty())
        {
            return {};
        }

        // Pick out the chunks that matter for searches
        std::vector<Chunk> relaventChunks;

        for (const Chunk &chunk : impl_->chunks)
        {
            for (const std::string &term : queryTerms)
            {
                std::size_t freq = impl_->index.term_frequency(term, chunk.id);

                if (freq > 0)
                {
                    relaventChunks.push_back(chunk);
                    break;
                }
            }
        }

        std::vector<SearchResult> results;

        double N = impl_->chunks.size();
        double Q = queryTerms.size();

        for (const Chunk &chunk : relaventChunks)
        {
            double baseScore = 0.0;
            std::size_t matched = 0;

            for (const std::string &term : queryTerms)
            {
                std::size_t freq = impl_->index.term_frequency(term, chunk.id);

                if (freq > 0)
                {
                    matched++;

                    double tf = 1.0 + std::log(freq);
                    double df = impl_->index.document_frequency(term);
                    double idf = std::log((N + 1.0) / (df + 1.0)) + 1.0;

                    baseScore += tf * idf;
                }
            }

            double coverage = 1.0 + 0.1 * matched / Q;
            double score = baseScore * coverage;

            score = std::round(score * 1e12) / 1e12;

            SearchResult curResult;
            curResult.chunk_id = chunk.id;
            curResult.document_id = chunk.document_id;
            curResult.chunk_sequence = chunk.sequence;
            curResult.text = chunk.text;
            curResult.score = score;
            curResult.matched_terms = matched;

            results.push_back(curResult);
        }

        std::sort(results.begin(), results.end(), [this](const SearchResult &res1, const SearchResult &res2)
                  {
                      // 1. Descending score
                      if (res1.score != res2.score)
                      {
                          return res1.score > res2.score;
                      }

                      auto chunk1 = std::find_if(impl_->chunks.begin(), impl_->chunks.end(), [&res1](const Chunk &chunk)
                                                 { return chunk.id == res1.chunk_id; });

                      auto chunk2 = std::find_if(impl_->chunks.begin(), impl_->chunks.end(), [&res2](const Chunk &chunk)
                                                 { return chunk.id == res2.chunk_id; });

                      if (chunk1->document_order != chunk2->document_order)
                      {
                          return chunk1->document_order < chunk2->document_order;
                      }

                      return res1.chunk_sequence < res2.chunk_sequence; });

        if (results.size() > static_cast<size_t>(k))
        {
            results.resize(k);
        }

        return results;
    }

    // Helper Function designed to remove repeated query words
    std::vector<std::string> ProcessingCore::normalizeQuery(const std::string &query) const
    {
        std::vector<std::string> rawTerms = TextProcessor::terms(query);

        std::vector<std::string> cleanTerms;

        for (size_t i = 0; i < rawTerms.size(); i++)
        {
            bool termAlreadyAdded = false;
            for (size_t j = 0; j < cleanTerms.size(); j++)
            {
                if (cleanTerms.at(j) == rawTerms.at(i))
                {
                    termAlreadyAdded = true;
                }
            }
            if (!termAlreadyAdded)
            {
                cleanTerms.push_back(rawTerms.at(i));
            }
        }

        return cleanTerms;
    }

    /*
    ContextItem elements:

    std::string chunk_id;
    std::string document_id;
    std::size_t chunk_sequence{};
    std::string text;
    std::size_t token_count{};
    double score{};
    bool truncated{};
    */

    std::vector<ContextItem> ProcessingCore::build_context(const std::string &query, int k, std::size_t token_budget) const
    {
        // TODO: build bounded context for the requested query.

        if (k < 0)
        {
            throw std::invalid_argument("Invalid Input: k is negative");
        }
        else if (k == 0 || token_budget == 0)
        {
            return {};
        }

        std::vector<std::string> queryTerms = normalizeQuery(query);

        if (queryTerms.empty())
        {
            return {};
        }

        std::vector<SearchResult> searchResults = search(query, k);

        std::vector<ContextItem> output;
        std::size_t tokensRemaining = token_budget;

        for (SearchResult curResult : searchResults)
        {

            // Retrieves the current Chunk
            Chunk curChunk;
            for (const Chunk &chunk : impl_->chunks)
            {
                if (curResult.chunk_id == chunk.id)
                {
                    curChunk = chunk;
                }
            }

            // If the entire chunk has less tokens than our remaining budget
            if (curChunk.token_count <= tokensRemaining)
            {
                ContextItem curItem;
                curItem.chunk_id = curResult.chunk_id;
                curItem.document_id = curResult.document_id;
                curItem.chunk_sequence = curResult.chunk_sequence;
                curItem.text = curChunk.text;
                curItem.token_count = curChunk.token_count;
                curItem.score = curResult.score;
                curItem.truncated = false;

                output.push_back(curItem);
                tokensRemaining -= curChunk.token_count;
            }
            else if (tokensRemaining != 0)
            {
                // Case for when the chunk has too many tokens
                std::vector<std::string> terms = TextProcessor::terms(curChunk.text);

                ContextItem curItem;
                curItem.chunk_id = curResult.chunk_id;
                curItem.document_id = curResult.document_id;
                curItem.chunk_sequence = curResult.chunk_sequence;
                curItem.text = TextProcessor::join(terms, 0, tokensRemaining);
                curItem.token_count = tokensRemaining;
                curItem.score = curResult.score;
                curItem.truncated = true;

                output.push_back(curItem);
                tokensRemaining = 0;
            }
        }

        return output;
    }

} // namespace aiws