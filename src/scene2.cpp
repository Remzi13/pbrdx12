#include "scene2.h"

#include <string>
#include <string_view>
#include <cctype>
#include <stdexcept>
#include <iostream>
#include <array>
#include <optional>
#include <vector>
#include <ostream>
#include <iosfwd>
#include <fstream>
#include <sstream>

#include "vector.h"

namespace {
	enum class TokenType {
		LBrace,		// {
		RBrace,		// }
		LBracket,	// [
		RBracket,	// ]
		Colon,		// :
		Comma,		// ,
		String,
		Number,
		Bool,
		End
	};

	struct Token {
		TokenType type;
		std::string_view text;
		size_t line;
		size_t column;
	};

	class Lexer
	{
	public:
		explicit Lexer(std::string_view input) : _input(input) 
		{
			double x = 0;
		}

		Token next()
		{
			skipWhitespace();

			if (_pos >= _input.size())
				return { TokenType::End, "", _line, _column };

			char c = _input[_pos];

			switch (c)
			{
			case '{': return simple(TokenType::LBrace);
			case '}': return simple(TokenType::RBrace);
			case '[': return simple(TokenType::LBracket);
			case ']': return simple(TokenType::RBracket);
			case ':': return simple(TokenType::Colon);
			case ',': return simple(TokenType::Comma);
			case '"': return string();
			default:
				if (std::isdigit(c) || c == '-')
					return number();
				else
					return boolean();
			}
			error("unexpect character");
			return { TokenType::End, "", _line, _column };
		}

	private:
		void skipWhitespace() 
		{
			while (_pos < _input.size()) {
				char c = _input[_pos];
				if (c == ' ' || c == '\t' || c == '\r') {
					advance();
				}
				else if (c == '\n') {
					_pos++;
					_line++;
					_column = 1;
				}
				else {
					break;
				}
			}
		}

		void advance() {
			_pos++;
			_column++;
		}


		Token simple(TokenType type) 
		{
			Token t{ type, _input.substr(_pos, 1), _line, _column };
			advance();
			return t;
		}

		Token string() 
		{
			size_t startLine = _line;
			size_t startCol = _column;

			size_t start = ++_pos;
			_column++;

			while (_pos < _input.size() && _input[_pos] != '"') {
				if (_input[_pos] == '\n')
				{
					error("newline in string");
				}
				advance();
			}

			if (_pos >= _input.size())
			{
				error("unterminated string");
			}

			auto text = _input.substr(start, _pos - start);
			advance();

			return { TokenType::String, text, startLine, startCol };		
		}

		Token boolean()
		{
			size_t startLine = _line;
			size_t startCol = _column;
			size_t start = _pos;

			while (_pos < _input.size() && std::isalpha(_input[_pos]))
				advance();

			return {
				TokenType::Bool,
				_input.substr(start, _pos - start),
				startLine,
				startCol
			};
		}

		Token number()
		{
			size_t startLine = _line;
			size_t startCol = _column;
			size_t start = _pos;

			if (_input[_pos] == '-')
				advance();

			while (_pos < _input.size() && std::isdigit(_input[_pos]))
				advance();

			if (_pos < _input.size() && _input[_pos] == '.') {
				advance();
				while (_pos < _input.size() && std::isdigit(_input[_pos]))
					advance();
			}

			if (_pos < _input.size() && (_input[_pos] == 'e' || _input[_pos] == 'E')) {
				advance();
				if (_input[_pos] == '+' || _input[_pos] == '-')
					advance();
				while (_pos < _input.size() && std::isdigit(_input[_pos]))
					advance();
			}

			return {
				TokenType::Number,
				_input.substr(start, _pos - start),
				startLine,
				startCol
			};
		}

		void error(const char* msg)
		{
			throw std::runtime_error(
				std::string(msg) +
				" at line " + std::to_string(_line) +
				", column " + std::to_string(_column)
			);
		}

	private:
		std::string_view _input;
		size_t _pos = 0;
		size_t _line = 1;
		size_t _column = 1;
	};


	class Parser {
		struct Node {
			std::string name;
			std::optional<int> camera;
			Vector3 translation{ 0,0,0 };
			Vector4 rotation{ 0,0,0,1 };
		};

		struct Camera {
			std::string name;
			float aspectRatio = 1.0f;
			float yfov = 0.7f;
			float znear = 0.1f;
			float zfar = 100.0f;
		};

		struct Scene {
			std::vector<int> nodes;
		};

		struct Material
		{
			std::string name;
			Vector3 emissiveFactor;
			float emissiveStrength;
			Vector4 baseColorFactor;
			float metallicFactor;
			float roughnessFactor;
		};

		

	public:
		struct SceneFile {
			int defaultScene = 0;
			std::vector<Scene> scenes;
			std::vector<Node> nodes;
			std::vector<Camera> cameras;
			std::vector<Material> materials;
		};

	public:
		Parser(Lexer& lexer) : _lexer(lexer) 
		{
			advance();
		}

		SceneFile parseSceneFile() {
			SceneFile file;
			expect(TokenType::LBrace);

			while (!match(TokenType::RBrace)) {
				std::string key = consumeString();
				expect(TokenType::Colon);

				if (key == "scene")
					file.defaultScene = consumeInt();
				else if (key == "scenes")
					parseScenes(file.scenes);
				else if (key == "nodes")
					parseNodes(file.nodes);
				else if (key == "cameras")
					parseCameras(file.cameras);
				else if (key == "materials")
					parseMaterials(file.materials);
				else
					skipValue();

				match(TokenType::Comma);
			}
			return file;
		}

	private:
		void advance()
		{
			_token = _lexer.next();
		}

		bool match(TokenType type)
		{
			if (_token.type == type)
			{
				advance();
				return true;
			}
			return false;		
		}

		void expect(TokenType type)
		{
			if (!match(type))
			{
				error("unexpected token");
			}
		}

		void skipValue() 
		{
			if (match(TokenType::String) || match(TokenType::Number))
				return;

			if (match(TokenType::LBrace))
			{
				while (!match(TokenType::RBrace)) {
					skipValue(); // key
					expect(TokenType::Colon);
					skipValue(); // value
					match(TokenType::Comma);
				}
				return;
			}

			if (match(TokenType::LBracket)) 
			{
				while (!match(TokenType::RBracket)) {
					skipValue();
					match(TokenType::Comma);
				}
				return;
			}

			error("invalid value");
		}

		std::string consumeString() 
		{
			if (_token.type != TokenType::String)
				error("expected string");
			std::string s(_token.text);
			advance();
			return s;
		}

		int consumeInt() 
		{
			if (_token.type != TokenType::Number)
				error("expected number");
			int v = std::stoi(std::string(_token.text));
			advance();
			return v;
		}

		float consumeFloat()
		{
			if (_token.type != TokenType::Number)
				error("expected number");
			float v = std::stof(std::string(_token.text));
			advance();
			return v;
		}

		Vector3 parseVec3() 
		{
			expect(TokenType::LBracket);
			Vector3 v{
				consumeFloat(),
				(expect(TokenType::Comma), consumeFloat()),
				(expect(TokenType::Comma), consumeFloat())
			};
			expect(TokenType::RBracket);
			return v;
		}

		Vector4 parseQuat() {
			expect(TokenType::LBracket);
			Vector4 q{
				consumeFloat(),
				(expect(TokenType::Comma), consumeFloat()),
				(expect(TokenType::Comma), consumeFloat()),
				(expect(TokenType::Comma), consumeFloat())
			};
			expect(TokenType::RBracket);
			return q;
		}

		Node parseNode() 
		{
			Node n;
			expect(TokenType::LBrace);

			while (!match(TokenType::RBrace)) {
				std::string key = consumeString();
				expect(TokenType::Colon);

				if (key == "name") {
					n.name = consumeString();
				}
				else if (key == "camera") {
					n.camera = consumeInt();
				}
				else if (key == "translation") {
					n.translation = parseVec3();
				}
				else if (key == "rotation") {
					n.rotation = parseQuat();
				}
				else {
					skipValue();
				}

				match(TokenType::Comma);
			}
			return n;
		}

		void parseNodes(std::vector<Node>& nodes) 
		{
			expect(TokenType::LBracket);
			while (!match(TokenType::RBracket)) {
				nodes.push_back(parseNode());
				match(TokenType::Comma);
			}
		}

		Scene parseScene()
		{
			Scene s;
			expect(TokenType::LBrace);

			while (!match(TokenType::RBrace)) {
				std::string key = consumeString();
				expect(TokenType::Colon);

				if (key == "nodes") {
					expect(TokenType::LBracket);
					while (!match(TokenType::RBracket)) {
						s.nodes.push_back(consumeInt());
						match(TokenType::Comma);
					}
				}
				else {
					skipValue();
				}

				match(TokenType::Comma);
			}
			return s;
		}

		void parseScenes(std::vector<Scene>& scenes)
		{
			expect(TokenType::LBracket);
			while (!match(TokenType::RBracket)) {
				scenes.push_back(parseScene());
				match(TokenType::Comma);
			}
		}

		void parsePerspective(Camera& c) 
		{
			expect(TokenType::LBrace);

			while (!match(TokenType::RBrace)) {
				std::string key = consumeString();
				expect(TokenType::Colon);

				if (key == "aspectRatio")
					c.aspectRatio = consumeFloat();
				else if (key == "yfov")
					c.yfov = consumeFloat();
				else if (key == "znear")
					c.znear = consumeFloat();
				else if (key == "zfar")
					c.zfar = consumeFloat();
				else
					skipValue();

				match(TokenType::Comma);
			}
		}

		Camera parseCamera() 
		{
			Camera c;
			expect(TokenType::LBrace);

			while (!match(TokenType::RBrace)) {
				std::string key = consumeString();
				expect(TokenType::Colon);

				if (key == "name")
					c.name = consumeString();
				else if (key == "perspective")
					parsePerspective(c);
				else
					skipValue();

				match(TokenType::Comma);
			}
			return c;
		}

		void parseCameras(std::vector<Camera>& cameras) 
		{
			expect(TokenType::LBracket);
			while (!match(TokenType::RBracket)) {
				cameras.push_back(parseCamera());
				match(TokenType::Comma);
			}
		}

		void parseExtensions(Material& m)
		{
			expect(TokenType::LBrace);

			while (!match(TokenType::RBrace)) {
				std::string key = consumeString();
				if (key == "KHR_materials_emissive_strength")
				{
					expect(TokenType::LBrace);
					while (!match(TokenType::RBrace)) {
						std::string key = consumeString();
						expect(TokenType::Colon);

						if (key == "emissiveStrength")
						{
							m.emissiveStrength = consumeFloat();
						}
						match(TokenType::Comma);
					}
				}
				match(TokenType::Comma);
			}
		}

		void parsePbrMetallicRoughness(Material& m)
		{
			expect(TokenType::LBrace);

			while (!match(TokenType::RBrace)) {
				std::string key = consumeString();
				expect(TokenType::Colon);

				if (key == "baseColorFactor")
					m.baseColorFactor = parseQuat();
				else if (key == "metallicFactor")
					m.metallicFactor = consumeFloat();
				else if (key == "roughnessFactor")
					m.roughnessFactor = consumeFloat();
				else
					skipValue();

				match(TokenType::Comma);
			}
		}

		Material parseMaterial()
		{
			Material m;
			expect(TokenType::LBrace);

			while (!match(TokenType::RBrace)) {
				std::string key = consumeString();
				expect(TokenType::Colon);

				if (key == "emissiveFactor")
					m.emissiveFactor = parseVec3();
				else if (key == "name")
					m.name = consumeString();
				else if (key == "pbrMetallicRoughness")
					parsePbrMetallicRoughness(m);
				else if (key == "extensions")
					parseExtensions(m);
				else if (key == "doubleSided")
					bool doubleSided = consumeString() == "true";
				else
					skipValue();

				match(TokenType::Comma);
			}
			return m;
		}

		void parseMaterials(std::vector<Material>& materials)
		{
			expect(TokenType::LBracket);
			while (!match(TokenType::RBracket)) {
				materials.push_back(parseMaterial());
				match(TokenType::Comma);
			}
		}

		void error(const char* msg)
		{

		}	

	private:
		Lexer& _lexer;
		Token _token;
	};
}

bool Scene2::parse(const char* fileName)
{
	Lexer lex(R"({ "x": [1, 2, 3] })");
	for (;;) {
		Token t = lex.next();
		std::cout << int(t.type) << " " << t.text << "\n";
		if (t.type == TokenType::End)
			break;
	}

	std::ifstream file(fileName);
	if (!file.is_open()) {
		std::cerr << "Can`t open file " << fileName << std::endl;
		return false;
	}

	std::stringstream buffer;
	buffer << file.rdbuf(); // Read the file buffer into the stream
	
	std::string s = buffer.str();
	Lexer lexer(s);
	Parser parser(lexer);
	Parser::SceneFile scene = parser.parseSceneFile();

	std::cout << "Nodes: " << scene.nodes.size() << "\n";
	std::cout << "Cameras: " << scene.cameras.size() << "\n";
	std::cout << "Materials: " << scene.materials.size() << "\n";


	return false;
}