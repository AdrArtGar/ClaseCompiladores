#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using u8    = std::uint8_t;
using u64   = std::uint64_t;
using idx_t = std::size_t;

enum struct Register : u8 {
	AL = 0,
	BL = 1,
};

constexpr std::string_view to_string(Register reg) noexcept {
	switch (reg) {
	case Register::AL:
		return "AL";
	case Register::BL:
		return "BL";
	default:
		return "INVALID";
	}
}

enum struct Opcode : u8 { MOV = 0, ADD, STO, END, INV };

constexpr std::string_view to_string(Opcode op) noexcept {
	switch (op) {
	case Opcode::MOV:
		return "MOV";
	case Opcode::ADD:
		return "ADD";
	case Opcode::STO:
		return "STO";
	case Opcode::END:
		return "END";
	default:
		return "INV";
	}
}

struct ISAAlphabet {
	static constexpr idx_t Aidx        = '9' - '0' + 1;
	static constexpr idx_t comma_idx   = 'Z' - 'A' + Aidx + 1;
	static constexpr idx_t space_idx   = comma_idx + 1;
	static constexpr idx_t invalid_idx = space_idx + 1;
	static constexpr idx_t num_symbols = invalid_idx + 1;
	static constexpr idx_t get_idx(char c) noexcept {
		if (c >= '0' && c <= '9') {
			return c - '0';
		} else if (c >= 'A' && c <= 'Z') {
			return c - 'A' + Aidx;
		} else
			switch (c) {
			case ',':
				return comma_idx;
			case ' ':
				return space_idx;
			default:
				return invalid_idx;
			}
	}
	static constexpr char from_idx(idx_t idx) {
		if (idx < Aidx) {
			return idx + '0';
		} else if (idx < comma_idx) {
			return idx - Aidx + 'A';
		} else
			switch (idx) {
			case comma_idx:
				return ',';
			case space_idx:
				return ' ';
			default:
				return '\0';
			}
	}
};

enum struct TokenType : u8 {
	MNEMONIC = 0,
	REGISTER = 1,
	NUMBER   = 2,
	COMMA    = 3,
};

constexpr std::string_view to_string(TokenType type) noexcept {
	switch (type) {
	case TokenType::MNEMONIC:
		return "MNEMONICO";
	case TokenType::REGISTER:
		return "REGISTRO";
	case TokenType::NUMBER:
		return "NUMERO";
	case TokenType::COMMA:
		return "COMA";
	default:
		return "UNKNOWN";
	}
}

struct Token {
	TokenType type;
	idx_t start;
	idx_t end;
};

std::string to_string(const Token& token, std::string_view input) {
	std::string result;

	result += to_string(token.type);
	result += "(\"";
	result += input.substr(token.start, token.end - token.start);
	result += "\")";

	return result;
}

struct MOV_inst {
	Register dst;
	u64 addr;
};

struct ADD_inst {
	Register op_a;
	Register op_b;
};

struct STO_inst {
	u64 addr;
};

struct END_inst {};

struct INV_inst {};

struct Instruction {
	Opcode inst = Opcode::INV;
	union {
		INV_inst inv{};
		MOV_inst mov;
		ADD_inst add;
		STO_inst sto;
		END_inst end;
	};
};

std::string to_string(const MOV_inst& inst) {
	using std::to_string;
	std::string res = "{registro: ";
	res += to_string(inst.dst);
	res += ", direccion: ";
	res += to_string(inst.addr);
	res += "}";
	return res;
}

std::string to_string(const ADD_inst& inst) {
	std::string res = "{registro: ";
	res += to_string(inst.op_a);
	res += ", registro: ";
	res += to_string(inst.op_b);
	res += "}";
	return res;
}

std::string to_string(const STO_inst& inst) {
	using std::to_string;
	std::string res = "{direccion: ";
	res += to_string(inst.addr);
	res += "}";
	return res;
}

std::string to_string(const END_inst&) {
	return "{}";
}

std::string to_string(const INV_inst&) {
	return "{}";
}

std::string to_string(const Instruction& inst) {
	std::string res(to_string(inst.inst));
	res += " ";
	switch (inst.inst) {
	case Opcode::MOV:
		res += to_string(inst.mov);
		break;

	case Opcode::ADD:
		res += to_string(inst.add);
		break;

	case Opcode::STO:
		res += to_string(inst.sto);
		break;

	case Opcode::END:
		res += to_string(inst.end);
		break;

	default:
		res += to_string(inst.inv);
		break;
	}
	return res;
}

struct DFAContext {
	idx_t currentTokenStart;
	Instruction instruction;
	std::vector<Token> tokens;
};

struct DFATransition {
	idx_t target_state;
	void (*func)(DFAContext&, idx_t, idx_t, char) = nullptr;
};

struct DFAState {
	std::string_view name;
	std::array<DFATransition, ISAAlphabet::num_symbols> transitions;
	bool terminal                                   = false;
	void (*on_input_end)(DFAContext&, idx_t, idx_t) = nullptr;
};

constexpr size_t num_states = 30;

constexpr idx_t q0 = 0;

constexpr idx_t qM           = 1;
constexpr idx_t qMO          = 2;
constexpr idx_t qMOV         = 3;
constexpr idx_t qMOV_esp     = 4;
constexpr idx_t qMOV_A       = 5;
constexpr idx_t qMOV_B       = 6;
constexpr idx_t qMOV_reg     = 7;
constexpr idx_t qMOV_precoma = 8;
constexpr idx_t qMOV_coma    = 9;
constexpr idx_t qMOV_num     = 10;

constexpr idx_t qA           = 11;
constexpr idx_t qAD          = 12;
constexpr idx_t qADD         = 13;
constexpr idx_t qADD_esp     = 14;
constexpr idx_t qADD_A       = 15;
constexpr idx_t qADD_AL      = 16;
constexpr idx_t qADD_precoma = 17;
constexpr idx_t qADD_coma    = 18;
constexpr idx_t qADD_B       = 19;
constexpr idx_t qADD_BL      = 20;

constexpr idx_t qS       = 21;
constexpr idx_t qST      = 22;
constexpr idx_t qSTO     = 23;
constexpr idx_t qSTO_esp = 24;
constexpr idx_t qSTO_num = 25;

constexpr idx_t qE   = 26;
constexpr idx_t qEN  = 27;
constexpr idx_t qEND = 28;

constexpr idx_t qERR = 29;

constexpr std::array<DFAState, num_states> dfa = [] {
	std::array<DFAState, num_states> temp;
	auto error_callback = [](DFAContext& ctx, idx_t state, idx_t pos, char c) {
		std::cout << "Carácter inválido: \'" << c
		          << "\' en la posición: " << pos
		          << " en el estado: " << dfa[state].name << std::endl;
		std::vector<char> valid;
		for (const auto [idx, t] :
		     std::ranges::views::enumerate(dfa[state].transitions))
			if (t.target_state != qERR)
				valid.emplace_back(ISAAlphabet::from_idx(idx));
		if (valid.empty()) {
			std::cout << "No se esperaban caracteres" << std::endl;
		} else if (valid.size() == 1) {
			std::cout << "Se esperaba \'" << valid[0] << "\'" << std::endl;
		} else {
			std::cout << "Se esperaba uno de:";
			bool first = true;
			for (auto c : valid) {
				if (!first)
					std::cout << ',';
				first = false;
				std::cout << " \'" << c << "\'";
			}
			std::cout << std::endl;
		}
		ctx.instruction = {};
	};
	auto unexpected_end_callback = [](DFAContext& ctx, idx_t state, idx_t pos) {
		std::cout << "Final inesperado de la entrada en la posición: " << pos
		          << " en el estado: " << dfa[state].name << std::endl;
		std::vector<char> valid;
		for (const auto [idx, t] :
		     std::ranges::views::enumerate(dfa[state].transitions))
			if (t.target_state != qERR)
				valid.emplace_back(ISAAlphabet::from_idx(idx));
		if (valid.empty()) {
			std::cout << "No se esperaban caracteres" << std::endl;
		} else if (valid.size() == 1) {
			std::cout << "Se esperaba \'" << valid[0] << "\'" << std::endl;
		} else {
			std::cout << "Se esperaba uno de:";
			bool first = true;
			for (auto c : valid) {
				if (!first)
					std::cout << ',';
				first = false;
				std::cout << " \'" << c << "\'";
			}
			std::cout << std::endl;
		}
		ctx.instruction = {};
	};
	auto token_start_callback = [](DFAContext& ctx, idx_t, idx_t pos, char) {
		ctx.currentTokenStart = pos;
	};
	DFATransition error_transition = {qERR, error_callback};
	// Initialize with error transition
	for (auto& state : temp) {
		for (auto& trans : state.transitions)
			trans = error_transition;
		state.on_input_end = unexpected_end_callback;
	}

	for (auto& trans : temp[qERR].transitions)
		trans = {qERR, nullptr};

	temp[q0].name = "q0";

	temp[qM].name           = "qM";
	temp[qMO].name          = "qMO";
	temp[qMOV].name         = "qMOV";
	temp[qMOV_esp].name     = "qMOV esp";
	temp[qMOV_A].name       = "qMOV A";
	temp[qMOV_B].name       = "qMOV B";
	temp[qMOV_reg].name     = "qMOV reg";
	temp[qMOV_precoma].name = "qMOV precoma";
	temp[qMOV_coma].name    = "qMOV coma";
	temp[qMOV_num].name     = "qMOV num";

	temp[qA].name           = "qA";
	temp[qAD].name          = "qAD";
	temp[qADD].name         = "qADD";
	temp[qADD_esp].name     = "qADD esp";
	temp[qADD_A].name       = "qADD A";
	temp[qADD_AL].name      = "qADD AL";
	temp[qADD_precoma].name = "qADD precoma";
	temp[qADD_coma].name    = "qADD coma";
	temp[qADD_B].name       = "qADD B";
	temp[qADD_BL].name      = "qADD BL";

	temp[qS].name       = "qS";
	temp[qST].name      = "qST";
	temp[qSTO].name     = "qSTO";
	temp[qSTO_esp].name = "qSTO esp";
	temp[qSTO_num].name = "qSTO num";

	temp[qE].name   = "qE";
	temp[qEN].name  = "qEN";
	temp[qEND].name = "qEND";

	temp[qERR].name         = "qERR";
	temp[qERR].on_input_end = nullptr;

	// ------------------------------------------------------------
	// MOV
	// ------------------------------------------------------------

	// q0 --M--> qM
	temp[q0].transitions[ISAAlphabet::get_idx('M')] = {qM,
	                                                   token_start_callback};

	// qM --O--> qMO
	temp[qM].transitions[ISAAlphabet::get_idx('O')] = {qMO, nullptr};

	// qMO --V--> qMOV
	temp[qMO].transitions[ISAAlphabet::get_idx('V')] = {qMOV, nullptr};

	// qMOV --space--> qMOV esp
	temp[qMOV].transitions[ISAAlphabet::get_idx(' ')] = {
	    qMOV_esp, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.instruction.inst = Opcode::MOV;
		    ctx.tokens.emplace_back(TokenType::MNEMONIC, ctx.currentTokenStart,
		                            pos);
	    }};

	// qMOV esp --space--> qMOV esp
	temp[qMOV_esp].transitions[ISAAlphabet::get_idx(' ')] = {qMOV_esp, nullptr};

	// qMOV esp --A--> qMOV A
	temp[qMOV_esp].transitions[ISAAlphabet::get_idx('A')] = {
	    qMOV_A, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.instruction.mov.dst = Register::AL;
		    ctx.currentTokenStart   = pos;
	    }};

	// qMOV esp --B--> qMOV B
	temp[qMOV_esp].transitions[ISAAlphabet::get_idx('B')] = {
	    qMOV_B, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.instruction.mov.dst = Register::BL;
		    ctx.currentTokenStart   = pos;
	    }};

	// qMOV A --L--> qMOV reg
	temp[qMOV_A].transitions[ISAAlphabet::get_idx('L')] = {qMOV_reg, nullptr};

	// qMOV B --L--> qMOV reg
	temp[qMOV_B].transitions[ISAAlphabet::get_idx('L')] = {qMOV_reg, nullptr};

	// qMOV reg --space--> qMOV precoma
	temp[qMOV_reg].transitions[ISAAlphabet::get_idx(' ')] = {
	    qMOV_precoma, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.tokens.emplace_back(TokenType::REGISTER, ctx.currentTokenStart,
		                            pos);
	    }};

	// qMOV reg --comma--> qMOV coma
	temp[qMOV_reg].transitions[ISAAlphabet::get_idx(',')] = {
	    qMOV_coma, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.tokens.emplace_back(TokenType::REGISTER, ctx.currentTokenStart,
		                            pos);
		    ctx.tokens.emplace_back(TokenType::COMMA, pos, pos + 1);
	    }};

	// qMOV precoma --space--> qMOV precoma
	temp[qMOV_precoma].transitions[ISAAlphabet::get_idx(' ')] = {qMOV_precoma,
	                                                             nullptr};

	// qMOV precoma --comma--> qMOV coma
	temp[qMOV_precoma].transitions[ISAAlphabet::get_idx(',')] = {
	    qMOV_coma, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.tokens.emplace_back(TokenType::COMMA, pos, pos + 1);
	    }};

	// qMOV coma --space--> qMOV coma
	temp[qMOV_coma].transitions[ISAAlphabet::get_idx(' ')] = {qMOV_coma,
	                                                          nullptr};

	// qMOV coma --digit--> qMOV num
	for (char c = '0'; c <= '9'; ++c) {
		temp[qMOV_coma].transitions[ISAAlphabet::get_idx(c)] = {
		    qMOV_num, [](DFAContext& ctx, idx_t, idx_t pos, char c) {
			    ctx.currentTokenStart    = pos;
			    ctx.instruction.mov.addr = c - '0';
		    }};
	}

	// qMOV num --digit--> qMOV num
	for (char c = '0'; c <= '9'; ++c) {
		temp[qMOV_num].transitions[ISAAlphabet::get_idx(c)] = {
		    qMOV_num, [](DFAContext& ctx, idx_t, idx_t, char c) {
			    ctx.instruction.mov.addr =
			        ctx.instruction.mov.addr * 10u + c - '0';
		    }};
	}

	temp[qMOV_num].terminal     = true;
	temp[qMOV_num].on_input_end = [](DFAContext& ctx, idx_t, idx_t pos) {
		ctx.tokens.emplace_back(TokenType::NUMBER, ctx.currentTokenStart, pos);
	};

	// ------------------------------------------------------------
	// ADD
	// ------------------------------------------------------------

	// q0 --A--> qA
	temp[q0].transitions[ISAAlphabet::get_idx('A')] = {qA,
	                                                   token_start_callback};

	// qA --D--> qAD
	temp[qA].transitions[ISAAlphabet::get_idx('D')] = {qAD, nullptr};

	// qAD --D--> qADD
	temp[qAD].transitions[ISAAlphabet::get_idx('D')] = {qADD, nullptr};

	// qADD --space--> qADD esp
	temp[qADD].transitions[ISAAlphabet::get_idx(' ')] = {
	    qADD_esp, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.instruction.inst = Opcode::ADD;
		    ctx.tokens.emplace_back(TokenType::MNEMONIC, ctx.currentTokenStart,
		                            pos);
	    }};

	// qADD esp --space--> qADD esp
	temp[qADD_esp].transitions[ISAAlphabet::get_idx(' ')] = {qADD_esp, nullptr};

	// qADD esp --A--> qADD A
	temp[qADD_esp].transitions[ISAAlphabet::get_idx('A')] = {
	    qADD_A, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.instruction.add.op_a = Register::AL;
		    ctx.currentTokenStart    = pos;
	    }};

	// qADD A --L--> qADD AL
	temp[qADD_A].transitions[ISAAlphabet::get_idx('L')] = {qADD_AL, nullptr};

	// qADD AL --space--> qADD precoma
	temp[qADD_AL].transitions[ISAAlphabet::get_idx(' ')] = {
	    qADD_precoma, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.tokens.emplace_back(TokenType::REGISTER, ctx.currentTokenStart,
		                            pos);
	    }};

	// qADD AL --comma--> qADD coma
	temp[qADD_AL].transitions[ISAAlphabet::get_idx(',')] = {
	    qADD_coma, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.tokens.emplace_back(TokenType::REGISTER, ctx.currentTokenStart,
		                            pos);

		    ctx.tokens.emplace_back(TokenType::COMMA, pos, pos + 1);
	    }};

	// qADD precoma --space--> qADD precoma
	temp[qADD_precoma].transitions[ISAAlphabet::get_idx(' ')] = {qADD_precoma,
	                                                             nullptr};

	// qADD precoma --comma--> qADD coma
	temp[qADD_precoma].transitions[ISAAlphabet::get_idx(',')] = {
	    qADD_coma, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.tokens.emplace_back(TokenType::COMMA, pos, pos + 1);
	    }};

	// qADD coma --space--> qADD coma
	temp[qADD_coma].transitions[ISAAlphabet::get_idx(' ')] = {qADD_coma,
	                                                          nullptr};

	// qADD coma --B--> qADD B
	temp[qADD_coma].transitions[ISAAlphabet::get_idx('B')] = {
	    qADD_B, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.instruction.add.op_b = Register::BL;
		    ctx.currentTokenStart    = pos;
	    }};

	// qADD B --L--> qADD BL
	temp[qADD_B].transitions[ISAAlphabet::get_idx('L')] = {qADD_BL, nullptr};

	temp[qADD_BL].terminal     = true;
	temp[qADD_BL].on_input_end = [](DFAContext& ctx, idx_t, idx_t pos) {
		ctx.tokens.emplace_back(TokenType::REGISTER, ctx.currentTokenStart,
		                        pos);
	};

	// ------------------------------------------------------------
	// STO
	// ------------------------------------------------------------

	// q0 --S--> qS
	temp[q0].transitions[ISAAlphabet::get_idx('S')] = {qS,
	                                                   token_start_callback};

	// qS --T--> qST
	temp[qS].transitions[ISAAlphabet::get_idx('T')] = {qST, nullptr};

	// qST --O--> qSTO
	temp[qST].transitions[ISAAlphabet::get_idx('O')] = {qSTO, nullptr};

	// qSTO --space--> qSTO esp
	temp[qSTO].transitions[ISAAlphabet::get_idx(' ')] = {
	    qSTO_esp, [](DFAContext& ctx, idx_t, idx_t pos, char) {
		    ctx.instruction.inst = Opcode::STO;

		    ctx.tokens.emplace_back(TokenType::MNEMONIC, ctx.currentTokenStart,
		                            pos);
	    }};

	// qSTO esp --space--> qSTO esp
	temp[qSTO_esp].transitions[ISAAlphabet::get_idx(' ')] = {qSTO_esp, nullptr};

	// qSTO esp --digit--> qSTO num
	for (char c = '0'; c <= '9'; ++c) {
		temp[qSTO_esp].transitions[ISAAlphabet::get_idx(c)] = {
		    qSTO_num, [](DFAContext& ctx, idx_t, idx_t pos, char c) {
			    ctx.currentTokenStart    = pos;
			    ctx.instruction.sto.addr = c - '0';
		    }};
	}

	// qSTO num --digit--> qSTO num
	for (char c = '0'; c <= '9'; ++c) {
		temp[qSTO_num].transitions[ISAAlphabet::get_idx(c)] = {
		    qSTO_num, [](DFAContext& ctx, idx_t, idx_t, char c) {
			    ctx.instruction.sto.addr =
			        ctx.instruction.sto.addr * 10u + c - '0';
		    }};
	}

	temp[qSTO_num].terminal     = true;
	temp[qSTO_num].on_input_end = [](DFAContext& ctx, idx_t, idx_t pos) {
		ctx.tokens.emplace_back(TokenType::NUMBER, ctx.currentTokenStart, pos);
	};

	// ------------------------------------------------------------
	// END
	// ------------------------------------------------------------

	// q0 --E--> qE
	temp[q0].transitions[ISAAlphabet::get_idx('E')] = {qE,
	                                                   token_start_callback};

	// qE --N--> qEN
	temp[qE].transitions[ISAAlphabet::get_idx('N')] = {qEN, nullptr};

	// qEN --D--> qEND
	temp[qEN].transitions[ISAAlphabet::get_idx('D')] = {qEND, nullptr};

	temp[qEND].terminal     = true;
	temp[qEND].on_input_end = [](DFAContext& ctx, idx_t, idx_t pos) {
		ctx.instruction.inst = Opcode::END;
		ctx.tokens.emplace_back(TokenType::MNEMONIC, ctx.currentTokenStart,
		                        pos);
	};

	return temp;
}();

void print_components(const Instruction& inst) {
	std::cout << "COMPONENTES IDENTIFICADOS" << std::endl;

	std::cout << "Mnemónico: " << to_string(inst.inst) << std::endl;
	switch (inst.inst) {
	case Opcode::MOV:
		std::cout << "Registro destino: " << to_string(inst.mov.dst)
		          << std::endl;
		std::cout << "Dirección de memoria: " << inst.mov.addr << std::endl;
		std::cout << "Direccionamiento: directo" << std::endl;
		break;

	case Opcode::ADD:
		std::cout << "Registro: " << to_string(inst.add.op_a) << std::endl;
		std::cout << "Registro: " << to_string(inst.add.op_b) << std::endl;
		break;

	case Opcode::STO:
		std::cout << "Dirección de memoria: " << inst.sto.addr << std::endl;
		std::cout << "Direccionamiento: directo" << std::endl;
		break;

	case Opcode::END:
		break;

	default:
		break;
	}
}

constexpr size_t num_moore_states = 9;

constexpr idx_t mMOV1 = 0;
constexpr idx_t mMOV2 = 1;
constexpr idx_t mMOV3 = 2;

constexpr idx_t mADD = 3;

constexpr idx_t mSTO1 = 4;
constexpr idx_t mSTO2 = 5;
constexpr idx_t mSTO3 = 6;

constexpr idx_t mEND = 7;

constexpr idx_t mFin = 8;

struct MooreState {
	idx_t target_state;
	std::string (*func)(const Instruction&);
};

std::array<MooreState, num_moore_states> moore_states = [] {
	std::array<MooreState, num_moore_states> temp;
	temp[mMOV1] = {mMOV2, [](const Instruction& inst) {
		               std::string res = "MAR <- ";
		               res += std::to_string(inst.mov.addr);
		               return res;
	               }};
	temp[mMOV2] = {mMOV3, [](const Instruction&) {
		               std::string res = "MBR <- M[MAR]";
		               return res;
	               }};
	temp[mMOV3] = {mFin, [](const Instruction& inst) {
		               std::string res(to_string(inst.mov.dst));
		               res += " <- MBR";
		               return res;
	               }};
	temp[mADD]  = {mFin, [](const Instruction& inst) {
		              std::string res = "ACC <- ";
		              res += to_string(inst.add.op_a);
		              res += " + ";
		              res += to_string(inst.add.op_b);
		              return res;
	               }};
	temp[mSTO1] = {mSTO2, [](const Instruction& inst) {
		               std::string res = "MAR <- ";
		               res += std::to_string(inst.sto.addr);
		               return res;
	               }};
	temp[mSTO2] = {mSTO3, [](const Instruction&) {
		               std::string res = "MBR <- ACC";
		               return res;
	               }};
	temp[mSTO3] = {mFin, [](const Instruction&) {
		               std::string res = "M[MAR] <- MBR";
		               return res;
	               }};
	temp[mEND]  = {mFin, [](const Instruction&) {
		              std::string res = "HALT <- 1";
		              return res;
	               }};
	temp[mFin] = {mFin, nullptr};
	return temp;
}();

struct Moore {
	idx_t state;
	Instruction inst;
	Moore(const Instruction& inst) : inst{inst} {
		switch (inst.inst) {
		case Opcode::MOV:
			state = mMOV1;
			break;
		case Opcode::ADD:
			state = mADD;
			break;
		case Opcode::STO:
			state = mSTO1;
			break;
		case Opcode::END:
			state = mEND;
			break;
		case Opcode::INV:
			state = mFin;
			break;
		}
	}
	std::string siguiente_paso() {
		auto& func = moore_states[state].func;
		state      = moore_states[state].target_state;
		return func(inst);
	}
};

void process_instruction(std::string_view input) {
	std::cout << "ENTRADA" << std::endl;
	std::cout << input << std::endl;
	std::cout << std::endl;

	std::cout << "VALIDACIÓN MEDIANTE AFD" << std::endl;
	DFAContext ctx{};
	idx_t state = q0;

	for (idx_t pos = 0; pos < input.size(); ++pos) {
		char c = input[pos];

		const auto& transition =
		    dfa[state].transitions[ISAAlphabet::get_idx(c)];

		if (transition.func)
			transition.func(ctx, state, pos, c);
		state = transition.target_state;
	}
	if (dfa[state].on_input_end)
		dfa[state].on_input_end(ctx, state, input.size());

	const bool valid = dfa[state].terminal;

	if (valid) {
		std::cout << "Instrucción válida." << std::endl;
		std::cout << std::endl;

		std::cout << "TOKENS RECONOCIDOS" << std::endl;
		for (const auto& token : ctx.tokens)
			std::cout << to_string(token, input) << std::endl;
		std::cout << std::endl;

		print_components(ctx.instruction);
		std::cout << std::endl;

		std::cout << "OBJETO INSTRUCCIÓN" << std::endl;
		std::cout << to_string(ctx.instruction) << std::endl;
		std::cout << std::endl;

		std::cout << "MICROOPERACIONES GENERADAS POR MOORE" << std::endl;
		{
			Moore moore(ctx.instruction);
			idx_t i = 1;
			while(moore.state != mFin) {
				std::cout << i++ << ". " << moore.siguiente_paso() << std::endl;
			}
		}
		std::cout << std::endl;
		std::cout << "Generación terminada." << std::endl;
	} else {
		std::cout << std::endl;
		std::cout << "No se construye el objeto instrucción." << std::endl;
		std::cout << "No se generan microoperaciones." << std::endl;
	}
}

int main() {
	constexpr std::array valid_inputs{"MOV AL, 6",
	                                  "MOV BL, 7",
	                                  "ADD AL, BL",
	                                  "STO 8",
	                                  "END",
	                                  "MOV AL, 0",
	                                  "MOV AL, 123456789",
	                                  "MOV   BL   ,   42",
	                                  "ADD   AL   ,   BL",
	                                  "STO 0"

	};

	constexpr std::array invalid_inputs{
	    "MOV AX, 6",  "MOV AL 6",    "ADD AL,",    "STO",     "END 8",
	    "MOV AL, 6,", "MOV AL, 6 7", "MOV AL, -1", "MOV AL,", "MOV AL",
	    "ADD BL, AL", "ADD AL, AL",  "END "};

	std::cout << "====================\n";
	std::cout << "VALID INPUTS\n";
	std::cout << "====================\n\n";

	for (const auto input : valid_inputs) {
		process_instruction(input);
		std::cout << '\n';
	}

	std::cout << "====================\n";
	std::cout << "INVALID INPUTS\n";
	std::cout << "====================\n\n";

	for (const auto input : invalid_inputs) {
		process_instruction(input);
		std::cout << '\n';
	}
	std::string line;
	while(std::getline(std::cin, line)) {
		process_instruction(line);
	}
}
