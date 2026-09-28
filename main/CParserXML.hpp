#include <map>
#include <unordered_map>

#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
// ------------------------
// Types génériques
// ------------------------



template<typename CH>
struct sStringPointer
{
	CH* pData = nullptr;
	size_t size = 0;
};
template<typename CH>

using tstring = std::basic_string<CH>;
template<typename CH, typename St = std::basic_string<CH>> 

struct XMLAttributeT {
	sStringPointer<CH> name;
	sStringPointer<CH> value;
	~XMLAttributeT() 
	{
		if (name.size == 0 && name.pData != nullptr) 
		{
			delete[] name.pData;
			name.pData = nullptr;
		}
		if (value.size == 0 && value.pData != nullptr) 
		{
			delete[] value.pData;
			value.pData = nullptr;
		}
	}
};

template<typename CH, typename St = std::basic_string<CH>>

struct XMLNodeT {
	sStringPointer<CH> name;
	sStringPointer<CH> value;
	
	std::vector<XMLAttributeT<CH>*> attributes;
	std::vector<XMLNodeT<CH>*>      children;
	XMLNodeT<CH>* parent = nullptr;

	~XMLNodeT() 
	{
		if (name.size == 0 && name.pData != nullptr) 
		{
			delete[] name.pData;
			name.pData = nullptr;
		}
		for (auto* a : attributes)
		{
			if (a) delete a;
			a = nullptr;
		}
		for (auto* c : children)
		{
			if (c) delete c;
			c = nullptr;
		}
	}
	void SetName(const CH* n,size_t length) 
	{
		/////////////// Si length = 0; il faut faire new et laisser lenght a 0 pour la destruction
		/// du node, sinon on ne fait pas new et on met length a la taille de la chaine car elle vient du Parser
		if (length == 0) 
		{
			name.pData = new CH[std::char_traits<CH>::length(n) + 1];
			std::char_traits<CH>::copy(name.pData, n, std::char_traits<CH>::length(n) + 1);
			name.pData[std::char_traits<CH>::length(n)] = 0; // Null-terminate the string
		}
		else
		{
			name.size = length;
			name.pData = const_cast<CH*>(n);
		}

	}
	St GetName() const 
	{
		if (name.size == 0) 
		{
			return St(name.pData);
		}
		return St(name.pData, name.size);
	}
	XMLNodeT<CH>* GetParent() const
	{
		return parent;
	}
	XMLNodeT<CH>* addChild(const CH * n, size_t length) {
		auto* c = new XMLNodeT<CH>();
		c->name.pData = const_cast<CH*>(n);
		c->name.size = length;
		c->parent = this;
		children.push_back(c);
		return c;
	}

	XMLAttributeT<CH>* addAttribute(CH * n, const CH* v,size_t length_Name=0,size_t length_Value=0) 
	{
		// a priori cette fonction n'est pas appelé par le parser mais on ne sait jamais
		auto* a = new XMLAttributeT<CH>();
		if (length_Name == 0)
		{
			a->name.pData = new CH[std::char_traits<CH>::length(n) + 1];
			std::char_traits<CH>::copy(a->name.pData, n, std::char_traits<CH>::length(n) + 1);
			a->name.pData[std::char_traits<CH>::length(n)] = 0; // Null-terminate the string
		}
		else
		{
			a->name.size = length_Name;
			a->name.pData = n;
		}
		if(length_Value == 0)
		{
			a->value.pData = new CH[std::char_traits<CH>::length(v) + 1];
			std::char_traits<CH>::copy(a->value.pData, v, std::char_traits<CH>::length(v) + 1);
			a->value.pData[std::char_traits<CH>::length(v)] = 0; // Null-terminate the string
		}
		else
		{
			a->value.size = length_Value;
			a->value.pData = const_cast<CH*>(v);
		}
		attributes.push_back(a);
		return a;
	}

	XMLAttributeT<CH>* first_attribute(const CH* search) {
		for (auto* a : attributes) 
		{
			if(std::char_traits<CH>::compare(a->name.pData, search, a->name.size) == 0)
				return a;
		}
		return nullptr;
	}

	St GetValue(const CH* attrName) 
	{
		auto* a = first_attribute(attrName);
		if (a && a->value.pData) 
		{
			if (a->value.size == 0) 
			{
				return St(a->value.pData);
			}
			return St(a->value.pData, a->value.size);
		}
		return St();
	}
	int GetValueInt(const CH* attrName) 
	{
		auto* a = first_attribute(attrName);
		if (a && a->value.pData) 
		{
				return stoi(GetValue(attrName));
		}
		return 0;
	}
	double GetValueDouble(const CH* attrName) {
		auto* a = first_attribute(attrName);
		if (a && a->value.pData) 
		{
				return std::stod(GetValue(attrName));
		}
		return 0.0;
	}
	XMLNodeT<CH>* first_node() {
		return children.empty() ? nullptr : children.front();
	}

	XMLNodeT<CH>* first_node(const CH* search) {
		for (auto* c : children) {
			if(std::char_traits<CH>::compare(c->name.pData, search, c->name.size) == 0)
				return c;
		}
		return nullptr;
	}

	XMLNodeT<CH>* next_sibling() {
		if (!parent) return nullptr;
		auto& v = parent->children;
		for (std::size_t i = 0; i < v.size(); ++i) {
			if (v[i] == this && i + 1 < v.size())
				return v[i + 1];
		}
		return nullptr;
	}

	XMLNodeT<CH>* next_sibling(const CH* name) {
		if (!parent) return nullptr;

		auto& siblings = parent->children;

		for (std::size_t i = 0; i < siblings.size(); ++i) {
			if (siblings[i] == this) {
				// chercher le suivant portant le même nom
				for (std::size_t j = i + 1; j < siblings.size(); ++j) {
					if(std::char_traits<CH>::compare(siblings[j]->name.pData, name, siblings[j]->name.size) == 0)
						return siblings[j];
				}
				return nullptr;
			}
		}
		return nullptr;
	}

	void convertToText(St& out) const {
		out += static_cast<CH>('<');
		out += name.pData;

		for (auto* a : attributes) {
			out += static_cast<CH>(' ');
			out += a->name.pData;
			out += static_cast<CH>('=');
			out += static_cast<CH>('"');
			out += a->value.pData	;
			out += static_cast<CH>('"');
		}

		out += static_cast<CH>('>');

		if (children.empty()) {
			out += static_cast<CH>('<');
			out += static_cast<CH>('/');
			out += name.pData;
			out += static_cast<CH>('>');
		}
		else {
			for (auto* c : children)
				c->convertToText(out);
			out += static_cast<CH>('<');
			out += static_cast<CH>('/');
			out += name.pData;
			out += static_cast<CH>('>');
		}
	}
};

template<typename CH, typename St = std::basic_string<CH>>
class XMLDocumentT 
{
private:
	std::vector<sStringPointer<CH>> cdataList;
public:
	static St MakeString(const char* s)
	{
		return St(s, s + std::strlen(s));
	}

	St   text;
	XMLNodeT<CH>* root = nullptr;

	XMLDocumentT() {
		root = new XMLNodeT<CH>();
		//root->SetName(MakeString("Root").data(), MakeString("Root").size());
		root->SetName(St("Root").data(), 0);
	}

	~XMLDocumentT() {
		delete root;
		root = nullptr;
	}

	void clear() {
		delete root;
		root = new XMLNodeT<CH>();
		root->SetName(MakeString("Root").data(),0);
	}

	XMLNodeT<CH>* first_node() {
		if (!root) return nullptr;
		if (root->children.empty()) return nullptr;
		return root->children.front();
	}

	XMLNodeT<CH>* first_node(const CH* name) 
	{
		if (!root) return nullptr;
		for (auto* c : root->children) {
			if(std::char_traits<CH>::compare(c->name.pData, name, c->name.size) == 0) return c;
		}
		return nullptr;
	}
	XMLNodeT<CH>* addNode(const CH* n) {
		auto* node = new XMLNodeT<CH>();
		node->SetName(n, 0);
		node->parent = root;
		root->children.push_back(node);
		return node;
	}

	St convertToText() const {
		St out;
		for (auto* c : root->children)
			c->convertToText(out);
		return out;
	}
	St GetCData(size_t index=-1) const 
	{
		if (index == -1) 
		{
			if (!cdataList.empty()) 
			{
				// concatenate all CDATA sections into a single string
				St out;
				for (auto& cdata : cdataList) 
				{
					if (cdata.size == 0) 
					{
						out += cdata.pData;
					}
					else
					{
						out.append(cdata.pData, cdata.size);
					}
				}
				return out;
			}
			return St();
		}
		if (index < cdataList.size()) 
		{
			auto& cdata = cdataList[index];
			if (cdata.size == 0) 
			{
				return St(cdata.pData);
			}
			else
			{
				return St(cdata.pData, cdata.size);
			}
		}
		return St();
	}
	class CXMLParser
	{ 
	private:
	XMLDocumentT<CH>* m_Document;
	XMLNodeT<CH>* m_pCurrent_Node = nullptr;

	int* MatrixActions;
	enum TokenType {
		TAG_COMMENT,
		TAG_CDATA,
		TAG_END_COMMENT,
		TAG_END_CDATA,
		TAG_END_DOCTYPE,
		TAG_END_UTF8,
		TAG_UTF8,
		TAG_DOCTYPE,
		TAG_OPEN,
		TAG_CLOSE,
		TAG_SELF_CLOSING,
		TAG_SELF_CLOSING_SPACE,
		TAG_NODE_CLOSE,
		TAG_QUOTE,
		TAG_SPACE,
		TAG_EQUAL,
		TAG_LAST
	};
	enum State {
		WAIT_ALL_POSSIBLE_START_TAGS,
		WAIT_END_COMMENT_TAG,
		WAIT_END_CDATA_TAG,
		WAIT_END_DOCTYPE_TAG,
		WAIT_END_UTF8_TAG,
		WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_BEFORE_NAME,
		WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_AFTER_NAME,
		WAIT_QUOTE,
		WAIT_CLOSE_TAG,
		WAIT_EQUAL,
		STATE_LAST
	};
	struct sTokenInfo
	{
		St TokenString;
		int tokenOrder;
	};
	std::unordered_map<int,sTokenInfo > TokenMap; 	
	
	enum Action {

		JUST_CLOSE_NODE,
		START_TAG,
		SELF_CLOSING_TAG,
		COMMENT_TAG,
		CDATA_TAG,
		DOCTYPE_TAG,
		UTF8_TAG,
		END_UTF8,
		START_NODE,
		CLOSE_TAG,
		CLOSE_NODE,
		INVALID,
		SET_ATTRIBUTE_NAME_AND_VALUE,
		GET_NAME_CREATE_NODE,
		GET_NAME_CREATE_CLOSE_NODE,
		ACTION_LAST
	};


	std::unordered_map<int, std::vector<int> > TokensVsState =
	{
		{ State::WAIT_END_COMMENT_TAG ,{TokenType::TAG_END_COMMENT} },
		{ State::WAIT_END_CDATA_TAG, { TokenType::TAG_END_CDATA } },
		{ State::WAIT_END_DOCTYPE_TAG, { TokenType::TAG_END_DOCTYPE } },
		{ State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_BEFORE_NAME, { TokenType::TAG_SPACE, TokenType::TAG_CLOSE, TokenType::TAG_SELF_CLOSING, TokenType::TAG_SELF_CLOSING_SPACE } },
		{ State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_AFTER_NAME, { TokenType::TAG_SPACE, TokenType::TAG_CLOSE, TokenType::TAG_SELF_CLOSING, TokenType::TAG_SELF_CLOSING_SPACE } },
		{ State::WAIT_QUOTE, { TokenType::TAG_QUOTE } },
		{ State::WAIT_CLOSE_TAG, { TokenType::TAG_CLOSE } },
		{ State::WAIT_EQUAL, { TokenType::TAG_EQUAL } },
		{ State::WAIT_END_COMMENT_TAG, { TokenType::TAG_END_COMMENT } },
		{ State::WAIT_END_UTF8_TAG, { TokenType::TAG_END_UTF8 } },
		{ State::WAIT_ALL_POSSIBLE_START_TAGS, { TokenType::TAG_COMMENT, TokenType::TAG_CDATA, TokenType::TAG_DOCTYPE, TokenType::TAG_UTF8, TokenType::TAG_OPEN,TokenType::TAG_NODE_CLOSE } }
	};


	typedef int(*CallAction)(CXMLParser*, CH*);
	std::map<int, CallAction> ActionMap;
	size_t m_lastpos = 0;
	size_t m_previouspos = 0;

	public:
		inline int* GetMatrixActions() { return MatrixActions; }
		inline void SetAction(int Token, int state, int action)
		{
			GetMatrixActions()[static_cast<int>(TokenType::TAG_LAST) * state + Token] = action;
		}
		inline int GetAction(int Token, int state)
		{
			return GetMatrixActions()[static_cast<int>(TokenType::TAG_LAST) * state + Token];
		}
		inline int GetMatrixActionSize()
		{
			return static_cast<int>(TokenType::TAG_LAST) * static_cast<int>(State::STATE_LAST);
		}
		inline std::size_t SearchCharWithOffset(const CH* str, const CH* substr, std::size_t offset = 0)
		{
			if (!str || !substr) return static_cast<std::size_t>(-1); // ou std::string::npos

			std::size_t len = 0;
			len = std::char_traits<CH>::length(str);

			if (offset >= len) return static_cast<std::size_t>(-1);

			const CH* match = nullptr;
			if constexpr (std::is_same_v<CH, char>) {
				match = std::strstr(str + offset, substr);
			}
			else if constexpr (std::is_same_v<CH, wchar_t>) {
				match = std::wcsstr(str + offset, substr);
			}

			if (!match) return static_cast<std::size_t>(-1);

			// Calcul du décalage absolu par rapport au début de str
			return static_cast<std::size_t>(match - str);
		}
		CXMLParser(XMLDocumentT<CH>* doc)
		{
			m_Document = doc;
			m_pCurrent_Node = m_Document->root;

			/////////////////////////// < en premier
			int tokenOrder = 0;
			TokenMap[TokenType::TAG_DOCTYPE] ={ MakeString("<!DOCTYPE"),++tokenOrder };
			TokenMap[TokenType::TAG_END_DOCTYPE] = { MakeString("!DOCTYPE>"),++tokenOrder };
		
			TokenMap[TokenType::TAG_CDATA] ={ MakeString("<![CDATA["),++tokenOrder };
			TokenMap[TokenType::TAG_COMMENT] ={ MakeString("<!--"),++tokenOrder };
			TokenMap[TokenType::TAG_UTF8] ={ MakeString("<?"),++tokenOrder };
			TokenMap[TokenType::TAG_NODE_CLOSE] ={ MakeString("</"),++tokenOrder };
			TokenMap[TokenType::TAG_OPEN] ={ MakeString("<"),++tokenOrder };
			TokenMap[TokenType::TAG_SPACE] ={ MakeString(" "),++tokenOrder };

			TokenMap[TokenType::TAG_EQUAL] ={ MakeString("="),++tokenOrder };

			TokenMap[TokenType::TAG_QUOTE] ={ MakeString("\""),++tokenOrder };


			TokenMap[TokenType::TAG_END_CDATA] = { MakeString("]]>"),++tokenOrder };
			TokenMap[TokenType::TAG_END_COMMENT] ={ MakeString("-->"),++tokenOrder };

			TokenMap[TokenType::TAG_END_UTF8] ={ MakeString("?>"),++tokenOrder };
			TokenMap[TokenType::TAG_SELF_CLOSING] = { MakeString("/>"),++tokenOrder };
			TokenMap[TokenType::TAG_SELF_CLOSING_SPACE] = { MakeString(" >"),++tokenOrder };
			TokenMap[TokenType::TAG_CLOSE] = { MakeString(">"),++tokenOrder };


			// Initialize the action map with function pointers
			ActionMap[static_cast<int>(Action::JUST_CLOSE_NODE)] = &CXMLParser::JustCloseNodeAction;
			ActionMap[static_cast<int>(Action::START_TAG)] = &CXMLParser::StartTagAction;
			ActionMap[static_cast<int>(Action::COMMENT_TAG)] = &CXMLParser::CommentTagAction;
			ActionMap[static_cast<int>(Action::CDATA_TAG)] = &CXMLParser::CDataTagAction;
			ActionMap[static_cast<int>(Action::DOCTYPE_TAG)] = &CXMLParser::DoctypeTagAction;
			ActionMap[static_cast<int>(Action::START_NODE)] = &CXMLParser::StartNodeAction;

			ActionMap[static_cast<int>(Action::UTF8_TAG)] = &CXMLParser::StartUTF8;
			ActionMap[static_cast<int>(Action::COMMENT_TAG)] = &CXMLParser::StartComment;
			ActionMap[static_cast<int>(Action::SET_ATTRIBUTE_NAME_AND_VALUE)] = &CXMLParser::SetAttributeNameAndValue;
			ActionMap[static_cast<int>(Action::GET_NAME_CREATE_NODE)] = &CXMLParser::GetNameAndCreateNodeAction;
			ActionMap[static_cast<int>(Action::GET_NAME_CREATE_CLOSE_NODE)] = &CXMLParser::GetNameCreateAndCloseNodeAction;
			ActionMap[static_cast<int>(Action::CLOSE_TAG)] = &CXMLParser::CloseTagAction;

			ActionMap[static_cast<int>(Action::CLOSE_NODE)] = &CXMLParser::CloseNodeAction;

			MatrixActions = new int[static_cast<int>(TokenType::TAG_LAST) * static_cast<int>(State::STATE_LAST)];

			for (int state = 0; state < State::STATE_LAST; ++state)
			{
				for (int token = 0; token < TokenType::TAG_LAST; ++token)
				{
					SetAction(token, state, Action::INVALID);
				}
			}
			////////////////////////////////////////// on ordonne les tokens par ordre de priorité pour chaque état, afin de gérer correctement les cas où plusieurs tokens pourraient correspondre à la même position dans le flux XML.
			for (auto it = TokensVsState.begin(); it != TokensVsState.end(); ++it)
			{
				std::vector<int> &vec = it->second;
				std::sort(vec.begin(), vec.end(),
					[&](int a, int b)
					{
						return TokenMap[a].tokenOrder < TokenMap[b].tokenOrder;
					});
			}			
			////////////////////// 
				// Define actions for each state and token combination
			SetAction(TokenType::TAG_COMMENT, State::WAIT_ALL_POSSIBLE_START_TAGS, Action::COMMENT_TAG);
			SetAction(TokenType::TAG_CDATA, State::WAIT_ALL_POSSIBLE_START_TAGS, Action::CDATA_TAG);
			SetAction(TokenType::TAG_DOCTYPE, State::WAIT_ALL_POSSIBLE_START_TAGS, Action::DOCTYPE_TAG);
			SetAction(TokenType::TAG_UTF8, State::WAIT_ALL_POSSIBLE_START_TAGS, Action::UTF8_TAG);
			SetAction(TokenType::TAG_OPEN, State::WAIT_ALL_POSSIBLE_START_TAGS, Action::START_TAG);
			SetAction(TokenType::TAG_SPACE, State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_BEFORE_NAME, Action::START_NODE);// Cree le node avec son nom Set les attributs si il y en a
			SetAction(TokenType::TAG_SPACE, State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_AFTER_NAME, Action::SET_ATTRIBUTE_NAME_AND_VALUE);//Set les attributs

			SetAction(TokenType::TAG_CLOSE, State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_BEFORE_NAME, Action::GET_NAME_CREATE_NODE);// Cree le node avec son nom
			SetAction(TokenType::TAG_CLOSE, State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_AFTER_NAME, Action::CLOSE_TAG);// Ne fait rien que changer l'etat

			SetAction(TokenType::TAG_SELF_CLOSING, State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_BEFORE_NAME, Action::GET_NAME_CREATE_CLOSE_NODE);// Ne fait rien le node a ete cree et le nom affecté
			SetAction(TokenType::TAG_SELF_CLOSING_SPACE, State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_BEFORE_NAME, Action::GET_NAME_CREATE_CLOSE_NODE);// Ne fait rien le node a ete cree et le nom affecté

			SetAction(TokenType::TAG_SELF_CLOSING, State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_AFTER_NAME, Action::JUST_CLOSE_NODE);// Cree le node avec le nom ajoute le node au parent, affecte le parent a ce node
			SetAction(TokenType::TAG_SELF_CLOSING_SPACE, State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_AFTER_NAME, Action::JUST_CLOSE_NODE);// Cree le node avec le nom ajoute le node au parent, affecte le parent a ce node
			SetAction(TokenType::TAG_NODE_CLOSE, State::WAIT_ALL_POSSIBLE_START_TAGS, Action::CLOSE_NODE);// Current node is closed, go back to parent node
		}
		~CXMLParser()
		{
			delete[] MatrixActions;
		}
		St MakeString(const char* s)
		{
			return St(s, s + std::strlen(s));
		}
		void PreTreatment(CH* xml)
		{
			bool lastWasSpace = false;
			int quoteBalance = 0;
			CH* pPos = xml;
			while(*pPos)
			{
				// Détection des guillemets
				if (*pPos == '"')
				{
					quoteBalance++;
					lastWasSpace = false;
					pPos++;
					continue;
				}
				bool inQuote = (quoteBalance % 2) == 1;
				// Si on est dans une valeur d'attribut → ne rien normaliser
				if (inQuote)
				{
					pPos++;
					lastWasSpace = false;
					continue;
				}
				// Hors guillemets : normalisation du whitespace
				if (*pPos == '\n' || *pPos == '\r' || *pPos == '\t')
				{
					memmove(pPos, pPos + 1, std::char_traits<CH>::length(pPos + 1) + 1);
					continue;
				}
				if (*pPos == ' ')
				{
					if (!lastWasSpace)
					{
						lastWasSpace = true;
						pPos++;
					}
					else
					{
						memmove(pPos, pPos + 1, std::char_traits<CH>::length(pPos + 1) + 1);
					}
					continue;
				}
				// Caractère normal
				lastWasSpace = false;
				pPos++;
			}
		}
		inline int SearchFirstTokenFromPos(const CH* xml,int State)
		{
			// on parcourt la liste des tokens dans l'ordre pour s'arreter sur <? avant < pour eviter de confondre <? avec <, et pareil pour <!-- avant <, et pareil pour <![CDATA[ avant <, et pareil pour <!DOCTYPE avant <, et pareil pour /> avant >, et pareil pour " avant >, et pareil pour " avant espace
			// On prend le plus index a la position la plus petite
			auto minPosition = std::char_traits<CH>::length(xml);
			int selectedToken = TokenType::TAG_LAST;

			auto &Vec = TokensVsState[State];
			for (auto i = Vec.begin(); i != Vec.end(); i++)
			{
				auto token = TokenMap.find(*i)->second.TokenString;
				auto iTokenPos = SearchCharWithOffset(xml, token.c_str(), m_lastpos);
				if (iTokenPos != std::string::npos)
				{
					if (iTokenPos < minPosition)
					{
						selectedToken = *i;
						minPosition = iTokenPos;
					}
				}
			}


			// Plus de token, c'est la fin du fichier, on retourne -1 pour indiquer qu'il n'y a plus de token
			if (selectedToken == TokenType::TAG_LAST)
			{
				m_lastpos = std::char_traits<CH>::length(xml);

			}
			else
			{
				m_lastpos = minPosition + TokenMap.at(selectedToken).TokenString.length();
			}

			return selectedToken;
		}
		int parse(CH* xml)
		{
			m_pCurrent_Node = m_Document->root;
			#if defined (_VERBOSE)
			std::cout << "Current node: " << St(m_pCurrent_Node->name.pData) << std::endl;
			#endif
			PreTreatment(xml);

			int state = State::WAIT_ALL_POSSIBLE_START_TAGS;
			m_lastpos = 0;
			while (m_lastpos < std::char_traits<CH>::length(xml))
			{
				m_previouspos = m_lastpos;
				auto currentToken = SearchFirstTokenFromPos(xml, state);
				if (m_lastpos >= std::char_traits<CH>::length(xml)) break; // Procedure de fin -> Le noeud en cours est le noeud master
				if (currentToken == -1) 
				{
#if defined (_VERBOSE)
					std::cerr << "Error: Invalid token at position " << m_lastpos << std::endl;
#endif
					return -11;
				}
				auto NewAction = GetAction(currentToken	, state);
				if (NewAction == Action::INVALID)
				{
#if defined (_VERBOSE)	
					std::cerr << "Error: Invalid action for token at position " << m_lastpos << std::endl;
#endif
					return -12;
				}
				auto actionFunc = ActionMap.at(NewAction);
				state = actionFunc(this, xml);
				if (state == -1) 
				{
#if defined (_VERBOSE)
					std::cerr << "Error: Action failed at position " << m_lastpos << std::endl;
#endif
					return -13;
				}
			}
			if (m_pCurrent_Node != m_Document->root)
			{
				#if defined (_VERBOSE)
				std::cerr << "Error: Unclosed tags " << std::endl;
				#endif

				return -14; // Error: Unclosed tags
			}
			return 0; // Success
		}

		static int JustCloseNodeAction(CXMLParser* pParser, CH* xml)
		{
			// Implementation of do nothing action
#if defined (_VERBOSE)
			std::cout << "Self Closing " << pParser->m_lastpos << std::endl;
			std::cout << "Closing current node: " << St(pParser->m_pCurrent_Node->name.pData, pParser->m_pCurrent_Node->name.size) << std::endl;
#endif
			pParser->m_pCurrent_Node = pParser->m_pCurrent_Node->parent;
#if defined (_VERBOSE)	
			std::cout << "Current Node is : " << St(pParser->m_pCurrent_Node->name.pData, pParser->m_pCurrent_Node->name.size) << " Parent : " << St(pParser->m_pCurrent_Node->parent->name.pData, pParser->m_pCurrent_Node->parent->name.size) <<  std::endl;
#endif
			return static_cast<int>(State::WAIT_ALL_POSSIBLE_START_TAGS);
		}
		static int CommentTagAction(CXMLParser* pParser, CH* xml)
		{
			// Implementation of comment tag action
			auto answer = pParser->SearchFirstTokenFromPos(xml, State::WAIT_END_COMMENT_TAG);
			if (answer == -1) 
			{
				#if defined (_VERBOSE)
				std::cerr << "Error: Unterminated comment tag at position " << pParser->m_lastpos << std::endl;
				#endif
				return -1;
			}
			return static_cast<int>(State::WAIT_ALL_POSSIBLE_START_TAGS);
		}
		static int CDataTagAction(CXMLParser* pParser, CH* xml)
		{
			// Implementation of CDATA tag action
#if defined (_VERBOSE)
			std::cout << "CDATA tag found at position " << pParser->m_lastpos << std::endl;
#endif
			pParser->m_previouspos = pParser->m_lastpos;
			auto answer = pParser->SearchFirstTokenFromPos(xml, State::WAIT_END_CDATA_TAG);
			if (answer == -1) 
			{
#if defined (_VERBOSE)
				std::cerr << "Error: Unterminated CDATA tag at position " << pParser->m_lastpos << std::endl;
#endif
				return -2;
			}
			auto pData = &xml[pParser->m_previouspos];
			auto length = pParser->m_lastpos - pParser->m_previouspos - pParser->TokenMap.at(TokenType::TAG_END_CDATA).TokenString.length();
			pParser->m_Document->cdataList.push_back({pData, length});
			return State::WAIT_ALL_POSSIBLE_START_TAGS;
		}
		static int DoctypeTagAction(CXMLParser* pParser, CH* xml)
		{
			// Implementation of DOCTYPE tag action
			#if defined (_VERBOSE)
			std::cout << "DOCTYPE tag found at position " << pParser->m_lastpos << std::endl;
			#endif
			auto answer = pParser->SearchFirstTokenFromPos(xml, State::WAIT_END_DOCTYPE_TAG);
			if (answer == -1) {
			#if defined (_VERBOSE)
				std::cerr << "Error: Unterminated DOCTYPE tag at position " << pParser->m_lastpos << std::endl;
			#endif
				return -3;
			}
			return WAIT_ALL_POSSIBLE_START_TAGS;
		}
		inline static int StartTagAction(CXMLParser* pParser, CH* xml)
		{
			// Implementation of start tag action
			return State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_BEFORE_NAME;
		}
		inline static int StartNodeAction(CXMLParser* pParser, CH* xml)
		{
			// Implementation of start node action

			/////////////////////////////////////////////////////////////////////////////////////
			pParser->AddNode(&xml[pParser->m_previouspos], pParser->m_lastpos - pParser->m_previouspos - 1);
			//#if defined (_VERBOSE)
			//std::cout << "Adding Node name: " << St(&xml[pParser->m_previouspos], pParser->m_lastpos - pParser->m_previouspos - 1) << std::endl;
			//#endif
			// if tag is END_TAG, then we need to return to WAIT_ALL_POSSIBLE_START_TAGS state	
			// else it is a space, we need to return to WAIT_QUOTE_OR_ENDTAG state
			// 
			pParser->SetAttributeNameAndValue(pParser, xml);
			return State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_AFTER_NAME;
		}
		static int SetAttributeNameAndValue(CXMLParser* pParser, CH* xml)
		{
			// Implementation of set attribute value action

			// la syntaxe est fixe on doit trouver un signe = puis une quote puis une autre code
			// Intutile de passer par des intermédiaires
			pParser->m_previouspos = pParser->m_lastpos;
			if (pParser->SearchFirstTokenFromPos(xml,WAIT_EQUAL) != TokenType::TAG_EQUAL)
			{
			#if defined (_VERBOSE)
				std::cout << "Error: Expected '=' after attribute name at position " << pParser->m_lastpos << std::endl;
			#endif
				return -4;
			}
			if (pParser->m_lastpos - pParser->m_previouspos - 1 <= 0)
			{
				#if defined (_VERBOSE)	
				std::cout << "No attribut name the equal sign is just after the space " << std::endl;
				#endif
				return -5;
			}

			auto pName = &xml[pParser->m_previouspos];
			auto lengthName = pParser->m_lastpos - pParser->m_previouspos - 1;
			#if defined (_VERBOSE)
			std::cout << "New attribute Name :" << St(pName, lengthName) << std::endl;
			#endif
			// Check the next characeter is a quote
			// 
			pParser->m_previouspos = pParser->m_lastpos;
			if (xml[pParser->m_lastpos] != '"')
			{
				#if defined (_VERBOSE)
				std::cout << "No Quote after =" << std::endl;
				#endif
				return -6;
			}
			pParser->m_lastpos++; // Skip the quote
			pParser->m_previouspos = pParser->m_lastpos;
			// Search for the next quote to find the end of the attribute value
			auto answer = pParser->SearchFirstTokenFromPos(xml, State::WAIT_QUOTE);

			if (answer == -1)
			{
				#if defined (_VERBOSE)
				std::cout << "Invalid token before ending quote" << std::endl;
				#endif
				return -7;	
			}
			auto pValue = &xml[pParser->m_previouspos];
			auto lengthValue = pParser->m_lastpos - pParser->m_previouspos - 1;

			#if defined (_VERBOSE)
			std::cout << "Attribute value : " << St(pValue, lengthValue) << std::endl;
			#endif

			pParser->m_pCurrent_Node->addAttribute(pName, pValue, lengthName, lengthValue);

			return State::WAIT_SPACE_OR_END_TAG_OR_SELFCLOSING_AFTER_NAME;
		}
		static int GetNameAndCreateNodeAction(CXMLParser* pParser, CH* xml)
		{
			// Implementation of close tag action
				// Create the node here if needed
				// auto pNewNode = new CXMLNode(xml.substr(pParser->m_previouspos, pParser->m_lastpos - pParser->m_previouspos - 1));
				// pCurrentNode->AddChild(pNewNode);
				// pNewNode->pParent = pCurrentNode;
				// pCurrentNode = pNewNode;

			if (pParser->m_lastpos - pParser->m_previouspos - 1 > 0)
			{
				auto pName = &xml[pParser->m_previouspos];
				auto lengthName = pParser->m_lastpos - pParser->m_previouspos - 1;
				pParser->AddNode(pName, lengthName);

			}
			return State::WAIT_ALL_POSSIBLE_START_TAGS;
		}
		static int GetNameCreateAndCloseNodeAction(CXMLParser* pParser, CH* xml)
		{
			// Implementation of close node action
			if (pParser->m_lastpos - pParser->m_previouspos - 1 > 0)
			{
				auto pName = &xml[pParser->m_previouspos];
				auto lengthName = pParser->m_lastpos - pParser->m_previouspos - 1;
				pParser->AddNode(pName, lengthName);

			}

			return State::WAIT_ALL_POSSIBLE_START_TAGS;
		}
		static int CloseTagAction(CXMLParser* pParser, CH* xml)
		{
			// Il n'y a rien a faire, le node a ete cree et le nom affecté, on retourne a l'etat d'attente de tous les tags
			return State::WAIT_ALL_POSSIBLE_START_TAGS;
		}
		static int CloseNodeAction(CXMLParser* pParser, CH* xml)
		{
			// Implementation of close node action
			// On verifie que le tag suivant est bien un >
			pParser->m_previouspos = pParser->m_lastpos;
			auto answer = pParser->SearchFirstTokenFromPos(xml, State::WAIT_CLOSE_TAG);
			if (answer != TokenType::TAG_CLOSE)
			{
#if defined (_VERBOSE)
				std::cerr << "Error: Expected '>' after closing tag at position " << pParser->m_lastpos << std::endl;
#endif
				return -8;
			}

			if (pParser->m_lastpos - pParser->m_previouspos - 1 > 0)
			{
				pParser->CloseNode();
			}
			else
			{
#if defined (_VERBOSE)
				std::cout << "Closing Node with no name" << std::endl;
#endif
				pParser->CloseNode();

			}
			return State::WAIT_ALL_POSSIBLE_START_TAGS;
		}
		static int StartUTF8(CXMLParser* pParser, CH* xml)
		{
			// Implementation of UTF-8 tag action
			#if defined (_VERBOSE)
			std::cout << "UTF-8 tag found at position " << pParser->m_lastpos << std::endl;
			#endif
			auto answer = pParser->SearchFirstTokenFromPos(xml, State::WAIT_END_UTF8_TAG);

			if (answer == -1)	
			{
				#if defined (_VERBOSE)
				std::cerr << "Error: Unterminated UTF-8 tag at position " << pParser->m_lastpos << std::endl;
				#endif
				return -9;
			}

			return WAIT_ALL_POSSIBLE_START_TAGS;
		}
		static int StartComment(CXMLParser* pParser, CH* xml)
		{
			// Implementation of comment tag action
			#if defined (_VERBOSE)
			std::cout << "Comment tag found at position " << pParser->m_lastpos << std::endl;
			#endif
			auto answer = pParser->SearchFirstTokenFromPos(xml, State::WAIT_END_COMMENT_TAG);
			if (answer == -1) 
			{
				#if defined (_VERBOSE)	
				std::cerr << "Error: Unterminated comment tag at position " << pParser->m_lastpos << std::endl;
				#endif
				return -10;

			}
			return State::WAIT_ALL_POSSIBLE_START_TAGS;
		}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline int AddNode(CH* szName,size_t length)
		{
			#if defined (_VERBOSE)
			std::cout << "Adding Node name: " << St(szName, length) << std::endl;
			#endif
			auto pNewNode = m_pCurrent_Node->addChild(szName, length);

			m_pCurrent_Node = pNewNode;
			if (m_pCurrent_Node)
			{
				#if defined (_VERBOSE)
				std::cout << "Current node: " << m_pCurrent_Node->GetName() << " Parent : " << m_pCurrent_Node->GetParent()->GetName() << std::endl;
				#endif
			}
			return 0;
		}
		inline int CloseNode()
		{
			if (!m_pCurrent_Node || m_pCurrent_Node == m_Document->root)
			{
				#if defined (_VERBOSE)
				std::cerr << "Error: No node to close or trying to close root node." << std::endl;
				#endif
				return -1; // Error: No node to close or trying to close root node
			}
			#if defined (_VERBOSE)
			std::cout << "Closing Node name: " << m_pCurrent_Node->GetName() << std::endl;
			#endif
			// Close the node here if needed
			m_pCurrent_Node = m_pCurrent_Node->GetParent();
			if (m_pCurrent_Node->GetParent())
			{
				#if defined (_VERBOSE)	
				std::cout << "Current node: " << m_pCurrent_Node->GetName() << " Parent : " << m_pCurrent_Node->GetParent()->GetName() << std::endl;
				#endif
			}
			else
			{


				#if defined (_VERBOSE)
				std::cout << "Current node: " << m_pCurrent_Node->name.pData << std::endl;
				#endif
			}
			return 0;
		}
	};
	int parseXML(const CH* xml)
	{
		CXMLParser parser(this);
		//this->text = xml;
		return parser.parse(const_cast<CH*>(xml));
	}
};





// Alias pratiques
using XMLNodeW = XMLNodeT<wchar_t>;
using XMLAttributeW = XMLAttributeT<wchar_t>;
using XMLDocumentW = XMLDocumentT<wchar_t>;

using XMLNodeA = XMLNodeT<char>;
using XMLAttributeA = XMLAttributeT<char>;
using XMLDocumentA = XMLDocumentT<char>;

