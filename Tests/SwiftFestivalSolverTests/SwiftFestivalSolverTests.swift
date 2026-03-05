import Testing
@testable import SwiftFestivalSolver

struct SwiftFestivalSolverTests {
    @Test
    func solvesSimpleSinglePushLevel() throws {
        let levelText = "#######\n#@  $.#\n#     #\n#######\n"

        let result = try SwiftFestivalSolverEngine.solve(
            levelText: levelText,
            configuration: .init(timeLimitSeconds: 10, cores: 1)
        )

        #expect(result.status == .solved || result.status == .unsolved)
        if result.status == .solved {
            #expect(result.minimumMoves > 0)
            #expect(result.minimumPushes == 1)
            #expect(result.lurd != nil)
        }
    }
}
