#include "upsm_solver_daemon.cpp"

#include <cassert>
#include <limits>

using namespace quantinnium::upsm;

int main() {
    std::string reason;
    InputFrame frame;
    frame.generation = 1;
    frame.nodes.resize(1);
    assert(validate_input(frame, reason));

    LargestMagnitudeDemoSolver solver;
    frame.nodes[0].state[3] = -4.0f;
    const auto actions = solver.solve(frame);
    assert(actions.size() == 1);
    assert(actions[0] == 3);

    assert(validate_actions(frame, actions, reason));
    assert(!validate_actions(frame, {}, reason));

    frame.nodes[0].state[0] = std::numeric_limits<float>::quiet_NaN();
    assert(!validate_input(frame, reason));

    frame.nodes[0].state[0] = 0.0f;
    frame.generation = 0;
    assert(!validate_input(frame, reason));

    frame.generation = 2;
    frame.nodes.clear();
    assert(!validate_input(frame, reason));
    return 0;
}
