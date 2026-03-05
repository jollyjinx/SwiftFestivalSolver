import CFestival
import Foundation

public struct FestivalSolveConfiguration: Hashable, Sendable {
    public var timeLimitSeconds: Int
    public var cores: Int

    public init(timeLimitSeconds: Int = 30, cores: Int = 1) {
        self.timeLimitSeconds = max(1, timeLimitSeconds)
        self.cores = max(1, cores)
    }
}

public enum FestivalSolveStatus: Hashable, Sendable {
    case solved
    case unsolved
}

public struct FestivalSolveResult: Hashable, Sendable {
    public let status: FestivalSolveStatus
    public let minimumMoves: Int
    public let minimumPushes: Int
    public let lurd: String?
    public let message: String?

    public init(
        status: FestivalSolveStatus,
        minimumMoves: Int,
        minimumPushes: Int,
        lurd: String?,
        message: String?
    ) {
        self.status = status
        self.minimumMoves = minimumMoves
        self.minimumPushes = minimumPushes
        self.lurd = lurd
        self.message = message
    }
}

public enum FestivalEngineError: Error, LocalizedError, Sendable {
    case invocationFailed(String)
    case solverError(String)
    case invalidSolvedResult

    public var errorDescription: String? {
        switch self {
        case let .invocationFailed(message):
            return message
        case let .solverError(message):
            return message
        case .invalidSolvedResult:
            return "Festival reported solved but did not provide a LURD solution."
        }
    }
}

public enum FestivalEngine {
    public static func solve(
        levelText: String,
        configuration: FestivalSolveConfiguration = .init()
    ) throws -> FestivalSolveResult {
        let solvedStatus = Int32(bitPattern: CFESTIVAL_STATUS_SOLVED.rawValue)
        let unsolvedStatus = Int32(bitPattern: CFESTIVAL_STATUS_UNSOLVED.rawValue)
        let errorStatus = Int32(bitPattern: CFESTIVAL_STATUS_ERROR.rawValue)

        var rawResult = CFestivalSolveResult(
            status: errorStatus,
            minimum_moves: 0,
            minimum_pushes: 0,
            lurd: nil,
            message: nil
        )
        defer { cfestival_free_result(&rawResult) }

        let invocationStatus = levelText.withCString { levelCString in
            cfestival_solve_level(
                levelCString,
                Int32(configuration.timeLimitSeconds),
                Int32(configuration.cores),
                &rawResult
            )
        }

        guard invocationStatus == 0 else {
            throw FestivalEngineError.invocationFailed(
                "Festival invocation failed with status \(invocationStatus)."
            )
        }

        let message = rawResult.message.map { String(cString: $0) }

        switch rawResult.status {
        case solvedStatus:
            guard let lurdPointer = rawResult.lurd else {
                throw FestivalEngineError.invalidSolvedResult
            }

            return FestivalSolveResult(
                status: .solved,
                minimumMoves: Int(rawResult.minimum_moves),
                minimumPushes: Int(rawResult.minimum_pushes),
                lurd: String(cString: lurdPointer),
                message: message
            )

        case unsolvedStatus:
            return FestivalSolveResult(
                status: .unsolved,
                minimumMoves: 0,
                minimumPushes: 0,
                lurd: nil,
                message: message
            )

        default:
            throw FestivalEngineError.solverError(message ?? "Festival failed with an unknown error.")
        }
    }
}
