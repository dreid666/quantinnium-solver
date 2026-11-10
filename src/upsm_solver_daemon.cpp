#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <optional>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>

namespace quantinnium::upsm {

inline constexpr std::size_t kMaxNodes = 4096;
inline constexpr std::size_t kStateDimension = 16;

struct NodeInput {
    std::array<float, kStateDimension> state{};
};

struct InputFrame {
    std::uint64_t generation{};
    std::vector<NodeInput> nodes;
};

enum class ResultStatus : std::uint32_t { committed = 1, failed = 2 };

struct SolverResult {
    std::uint64_t generation{};
    ResultStatus status{ResultStatus::failed};
    std::vector<std::uint32_t> actions;
    std::string error;
};

class FrameTransport {
public:
    virtual ~FrameTransport() = default;
    // Must be bounded and interruptible. Returned frame must own a private snapshot.
    virtual std::optional<InputFrame> wait_for_frame(std::chrono::milliseconds timeout,
                                                      std::stop_token stop) = 0;
    // Must publish and acknowledge exactly this generation; never publish partial output.
    virtual bool publish_result(const SolverResult& result) = 0;
};

class Solver {
public:
    virtual ~Solver() = default;
    virtual std::vector<std::uint32_t> solve(const InputFrame& frame) = 0;
};

// Demonstration-only smoke-test logic. Not a domain-specific solver.
class LargestMagnitudeDemoSolver final : public Solver {
public:
    std::vector<std::uint32_t> solve(const InputFrame& frame) override {
        std::vector<std::uint32_t> actions;
        actions.reserve(frame.nodes.size());
        for (const auto& node : frame.nodes) {
            const auto best = std::max_element(node.state.begin(), node.state.end(),
                [](float lhs, float rhs) { return std::abs(lhs) < std::abs(rhs); });
            actions.push_back(static_cast<std::uint32_t>(best - node.state.begin()));
        }
        return actions;
    }
};

inline bool validate_input(const InputFrame& frame, std::string& reason) {
    if (frame.generation == 0) {
        reason = "generation must be nonzero";
        return false;
    }
    if (frame.nodes.empty() || frame.nodes.size() > kMaxNodes) {
        reason = "node count outside permitted bounds";
        return false;
    }
    for (std::size_t i = 0; i < frame.nodes.size(); ++i) {
        for (std::size_t j = 0; j < frame.nodes[i].state.size(); ++j) {
            if (!std::isfinite(frame.nodes[i].state[j])) {
                reason = "non-finite value at node " + std::to_string(i) +
                         ", index " + std::to_string(j);
                return false;
            }
        }
    }
    return true;
}

inline bool validate_actions(const InputFrame& frame,
                             const std::vector<std::uint32_t>& actions,
                             std::string& reason) {
    if (actions.size() != frame.nodes.size()) {
        reason = "solver returned wrong action count";
        return false;
    }
    for (const auto action : actions) {
        if (action >= kStateDimension) {
            reason = "solver returned out-of-range action";
            return false;
        }
    }
    return true;
}

// A domain verifier should be added before treating any result as operationally committed.
inline void run_worker(FrameTransport& transport,
                       Solver& solver,
                       std::stop_token stop,
                       std::chrono::milliseconds poll_interval = std::chrono::milliseconds{250}) {
    while (!stop.stop_requested()) {
        auto frame = transport.wait_for_frame(poll_interval, stop);
        if (!frame) continue;

        SolverResult result;
        result.generation = frame->generation;
        std::string reason;
        try {
            if (!validate_input(*frame, reason)) {
                result.error = "input rejected: " + reason;
            } else {
                auto candidate = solver.solve(*frame);
                if (!validate_actions(*frame, candidate, reason)) {
                    result.error = "result rejected: " + reason;
                } else {
                    result.actions = std::move(candidate);
                    result.status = ResultStatus::committed;
                }
            }
        } catch (const std::exception& ex) {
            result.error = std::string("solver exception: ") + ex.what();
        } catch (...) {
            result.error = "unknown solver exception";
        }

        if (!transport.publish_result(result)) return;
    }
}

} // namespace quantinnium::upsm
