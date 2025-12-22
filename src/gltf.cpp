#include "gltf.h"

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
#include <map>
#include <variant>

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
			std::string name;
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

		struct Mesh
		{
			struct Primitive {
				size_t material;
				size_t indices;
				std::map<std::string, size_t> attributes;
			};
			std::string name;
			std::vector<Primitive> primitives;
		};

		struct Accessor
		{
			size_t bufferView;
			size_t componentType;
			size_t count;
			Vector3 max;
			Vector3 min;
			std::string type;
		};

		struct BufferView
		{
			size_t buffer;
			size_t byteLength;
			size_t byteOffset;
			size_t target;
		};

		struct Buffer
		{
			size_t byteLength;
			std::string uri;
		};

	public:
		struct SceneFile {
			int defaultScene = 0;
			std::vector<Scene> scenes;
			std::vector<Node> nodes;
			std::vector<Camera> cameras;
			std::vector<Material> materials;
			std::vector<Mesh> meshes;
			std::vector<Accessor> accessors;
			std::vector<BufferView> bufferViews;
			std::vector<Buffer> buffers;
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
				else if (key == "meshes")
					parseMeshes(file.meshes);
				else if (key == "accessors")
					parseAccessors(file.accessors);
				else if (key == "bufferViews")
					parseBufferViews(file.bufferViews);
				else if (key == "buffers")
					parseBuffers(file.buffers);
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

		bool check(TokenType type) const
		{
			return _token.type == type;
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

		class Element {
		public:

			using type = std::variant<
				bool,
				int,
				float,
				Vector3,
				Vector4,
				std::string,
				Element,
				std::vector<Element>,
				std::vector<float>
			>;

			Element() = default;


			void add(const std::string& name, type value) {
				_attributes[name] = std::move(value);
			}

			type* get(const std::string& name) {
				auto it = _attributes.find(name);
				if (it != _attributes.end()) {
					return &(it->second);
				}
				return nullptr;
			}

			bool has(const std::string& name) const {
				return _attributes.find(name) != _attributes.end();
			}

			template<typename T>
			T getAs(const std::string& name, T defaultValue = T()) const {
				auto it = _attributes.find(name);
				if (it != _attributes.end()) {
					if (auto valPtr = std::get_if<T>(&it->second)) {
						return *valPtr;
					}
				}
				return defaultValue;
			}

			std::vector<std::string> names() const
			{
				std::vector<std::string> v;
				for (const auto& attr : _attributes)
				{
					v.push_back(attr.first);
				}
				return v;
			}

		private:
			std::map<std::string, type> _attributes;
		};

		Element::type parseArray()
		{
			expect(TokenType::LBracket);
			if (check(TokenType::Number))
			{
				std::vector<float> values;
				while (!match(TokenType::RBracket))
				{
					values.push_back(consumeFloat());
					match(TokenType::Comma);
				}

				if (values.size() == 3) {
					return Vector3{ values[0], values[1], values[2] };
				}
				else if (values.size() == 4) {
					return Vector4{ values[0], values[1], values[2], values[3] };
				}

				return values;
			}
			else
			{
				std::vector<Element> values;
				while (!match(TokenType::RBracket))
				{
					values.push_back(parseElement());
					match(TokenType::Comma);
				}
				return values;
			}
		}

		Element::type parseValue()
		{
			switch (_token.type) {
			case TokenType::Number:
				return consumeFloat();

			case TokenType::String:
				return consumeString();

			case TokenType::LBrace:
				return parseElement();

			case TokenType::LBracket:
				return parseArray();

			case TokenType::Bool:
				return consumeString() == "true";

			default:
				throw std::runtime_error("Unexpected token type");
			}
		}

		Element parseElement()
		{
			expect(TokenType::LBrace);
			Element el;
			while (!match(TokenType::RBrace)) {
				std::string key = consumeString();
				expect(TokenType::Colon);

				el.add(key, parseValue());

				match(TokenType::Comma);
			}
			return el;
		}

		void parseCameras(std::vector<Camera>& cameras)
		{
			std::vector<Element> elements = std::get<std::vector<Element>>(parseValue());
			for (const auto& el : elements)
			{
				Camera c;
				c.name = el.getAs<std::string>("name", "None");
				c.aspectRatio = el.getAs<Element>("perspective").getAs<float>("aspectRatio", 1.0);
				c.yfov = el.getAs<Element>("perspective").getAs<float>("yfov", 1.0);
				c.znear = el.getAs<Element>("perspective").getAs<float>("znear", 1.0);
				c.zfar = el.getAs<Element>("perspective").getAs<float>("zfar", 1.0);

				cameras.push_back(c);
			}
		}

		void parseMaterials(std::vector<Material>& materials)
		{
			std::vector<Element> elements = std::get<std::vector<Element>>(parseValue());
			for (const auto& el : elements)
			{
				Material m;
				m.name = el.getAs<std::string>("name", "None");
				m.emissiveFactor = el.getAs<Vector3>("emissiveFactor");
				m.baseColorFactor = el.getAs<Element>("pbrMetallicRoughness").getAs<Vector4>("baseColorFactor");
				m.metallicFactor = el.getAs<Element>("pbrMetallicRoughness").getAs<float>("metallicFactor");
				m.roughnessFactor = el.getAs<Element>("pbrMetallicRoughness").getAs<float>("roughnessFactor");
				m.emissiveStrength = el.getAs<Element>("extensions").getAs<Element>("KHR_materials_emissive_strength").getAs<float>("emissiveStrength");

				materials.push_back(m);
			}
		}

		void parseMeshes(std::vector<Mesh>& meshes)
		{
			std::vector<Element> elements = std::get<std::vector<Element>>(parseValue());
			for (const auto& el : elements)
			{
				Mesh m;
				m.name = el.getAs<std::string>("name", "None");
				for (const auto& pr : el.getAs<std::vector<Element>>("primitives"))
				{
					Mesh::Primitive p;
					p.indices = (int)pr.getAs<float>("indices");
					p.material = (int)pr.getAs<float>("material");

					auto attrs = pr.getAs<Element>("attributes");

					for (const auto& name : attrs.names())
					{
						p.attributes.emplace(name, (int)attrs.getAs<float>(name.c_str()));
					}

					m.primitives.push_back(p);
				}
				meshes.push_back(m);
			}
		}

		void parseNodes(std::vector<Node>& nodes)
		{
			std::vector<Element> elements = std::get<std::vector<Element>>(parseValue());
			for (const auto& el : elements)
			{
				Node n;
				n.name = el.getAs<std::string>("name");
				if (el.has("camera"))
				{
					n.camera = (int)el.getAs<float>("camera");
				}
				n.rotation = el.getAs<Vector4>("rotation");
				n.translation = el.getAs<Vector3>("translation");

				nodes.push_back(n);
			}
		}

		void parseScenes(std::vector<Scene>& scenes)
		{
			std::vector<Element> elements = std::get<std::vector<Element>>(parseValue());
			for (const auto& el : elements)
			{
				Scene s;
				s.name = el.getAs<std::string>("name", "None");
				for (const auto& n : el.getAs<std::vector<float>>("nodes"))
				{
					s.nodes.push_back((int)n);
				}
				scenes.push_back(s);
			}
		}

		void parseAccessors(std::vector<Accessor>& accessors)
		{
			std::vector<Element> elements = std::get<std::vector<Element>>(parseValue());
			for (const auto& el : elements)
			{
				Accessor accessor;
				accessor.bufferView = (int)el.getAs<float>("bufferView");
				accessor.componentType = (int)el.getAs<float>("componentType");
				accessor.count = (int)el.getAs<float>("count");
				accessor.max = el.getAs<Vector3>("max");
				accessor.min = el.getAs<Vector3>("min");
				accessor.type = el.getAs<std::string>("type");

				accessors.push_back(accessor);
			}
		}

		void parseBufferViews(std::vector<BufferView>& bufferViews)
		{
			std::vector<Element> elements = std::get<std::vector<Element>>(parseValue());
			for (const auto& el : elements)
			{
				BufferView bf;
				bf.buffer = (int)el.getAs<float>("buffer");
				bf.byteLength = (int)el.getAs<float>("byteLength");
				bf.byteOffset = (int)el.getAs<float>("byteOffset");
				bf.target = (int)el.getAs<float>("target");
				bufferViews.push_back(bf);
			}
		}

		void parseBuffers(std::vector<Buffer>& buffers)
		{
			std::vector<Element> elements = std::get<std::vector<Element>>(parseValue());
			for (const auto& el : elements)
			{
				Buffer b;
				b.byteLength = (int)el.getAs<float>("byteLength");
				b.uri = el.getAs<std::string>("uri");
				buffers.push_back(b);
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

namespace gltf {

	bool parse(const char* fileName)
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

		std::cout << "Scenes" << scene.scenes.size() << "\n";
		std::cout << "Nodes: " << scene.nodes.size() << "\n";
		std::cout << "Cameras: " << scene.cameras.size() << "\n";
		std::cout << "Materials: " << scene.materials.size() << "\n";
		std::cout << "Meshes: " << scene.meshes.size() << "\n";
		std::cout << "Accessors" << scene.accessors.size() << "\n";
		std::cout << "BufferViews" << scene.bufferViews.size() << "\n";
		std::cout << "Buffers" << scene.buffers.size() << "\n";


		return false;
	}
}