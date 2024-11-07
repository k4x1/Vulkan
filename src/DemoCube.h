static std::array<VertexStandard, 36> demo_cube = {
    // -X side
	VertexStandard({-1.0f,-1.0f,-1.0f}, {0.0f, 1.0f}),
    VertexStandard({-1.0f,-1.0f, 1.0f}, {1.0f, 1.0f}),
    VertexStandard({-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}),
    VertexStandard({-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}),
    VertexStandard({-1.0f, 1.0f,-1.0f}, {0.0f, 0.0f}),
    VertexStandard({-1.0f,-1.0f,-1.0f}, {0.0f, 1.0f}),
    // -Z side
    VertexStandard({-1.0f,-1.0f,-1.0f}, {1.0f, 1.0f}),
    VertexStandard({ 1.0f, 1.0f,-1.0f}, {0.0f, 0.0f}),
    VertexStandard({ 1.0f,-1.0f,-1.0f}, {0.0f, 1.0f}),
    VertexStandard({-1.0f,-1.0f,-1.0f}, {1.0f, 1.0f}),
    VertexStandard({-1.0f, 1.0f,-1.0f}, {1.0f, 0.0f}),
    VertexStandard({ 1.0f, 1.0f,-1.0f}, {0.0f, 0.0f}),
    // -Y side
    VertexStandard({-1.0f,-1.0f,-1.0f}, {1.0f, 0.0f}),
    VertexStandard({ 1.0f,-1.0f,-1.0f}, {0.0f, 0.0f}),
    VertexStandard({ 1.0f,-1.0f, 1.0f}, {0.0f, 1.0f}),
    VertexStandard({-1.0f,-1.0f,-1.0f}, {1.0f, 0.0f}),
    VertexStandard({ 1.0f,-1.0f, 1.0f}, {0.0f, 1.0f}),
    VertexStandard({-1.0f,-1.0f, 1.0f}, {1.0f, 1.0f}),
    // +Y side
    VertexStandard({-1.0f, 1.0f,-1.0f}, {0.0f, 0.0f}),
    VertexStandard({-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}),
    VertexStandard({ 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}),
    VertexStandard({-1.0f, 1.0f,-1.0f}, {0.0f, 0.0f}),
    VertexStandard({ 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}),
    VertexStandard({ 1.0f, 1.0f,-1.0f}, {1.0f, 0.0f}),
    // +X side
    VertexStandard({ 1.0f, 1.0f,-1.0f}, {1.0f, 0.0f}),
    VertexStandard({ 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}),
    VertexStandard({ 1.0f,-1.0f, 1.0f}, {0.0f, 1.0f}),
    VertexStandard({ 1.0f,-1.0f, 1.0f}, {0.0f, 1.0f}),
    VertexStandard({ 1.0f,-1.0f,-1.0f}, {1.0f, 1.0f}),
    VertexStandard({ 1.0f, 1.0f,-1.0f}, {1.0f, 0.0f}),
    // +Z side
    VertexStandard({-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}),
    VertexStandard({-1.0f,-1.0f, 1.0f}, {0.0f, 1.0f}),
    VertexStandard({ 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}),
    VertexStandard({-1.0f,-1.0f, 1.0f}, {0.0f, 1.0f}),
    VertexStandard({ 1.0f,-1.0f, 1.0f}, {1.0f, 1.0f}),
    VertexStandard({ 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}),
};
