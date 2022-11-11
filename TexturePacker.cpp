#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <thread>
#include <atomic>

#define MAX_PATH 4096

namespace util
{
	bool isInteger(const std::string& str)
	{
		if(str.size() >= 1 && str[0] != '-' && !isdigit(str[0]))
			return false;

		for (size_t i = 1; i < str.size(); ++i)
			if (!isdigit(str[i]))
				return false;

		return true;
	}
}

namespace os
{
	//https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamea
	//https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms646932(v=vs.85)
	//https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessage
	class OpenFileDialog
	{
		bool _started = false;
		std::atomic<bool> _ready;
		wchar_t _path[MAX_PATH] = { 0 };
		std::thread *_thr = nullptr;
		OPENFILENAME _ofn = { sizeof(OPENFILENAME) };

		static void _kern(OPENFILENAME *ofn, std::atomic<bool>* ready)
		{
			GetOpenFileNameW(ofn);
			*ready = true;
		}

	public:
		OpenFileDialog(const HWND& hwnd, const wchar_t *filter = L"All files\0*.*\0", const wchar_t *defaultExtension = nullptr, const bool allowMultiple = false)
		{
			_ofn.hwndOwner = hwnd;
			_ofn.lpstrTitle = L"Open Atlas";
			_ofn.lpstrFilter = filter;
			_ofn.lpstrDefExt = L".txt";
			_ofn.lpstrFile = _path;
			_ofn.nMaxFile = MAX_PATH;
			_ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
			if (allowMultiple) _ofn.Flags = OFN_ALLOWMULTISELECT;
		}

		void kill()
		{
			if (_thr)
			{
				_started = false;
				_thr->detach();
				delete _thr;
				_thr = nullptr;
			}
		}

		void start()
		{
			kill();
			memset(_path, 0, MAX_PATH);
			_ready = false;
			_started = true;
			_thr = new std::thread(_kern, &_ofn, &_ready);
		}

		bool isReady() const { return _ready.load(); }
		bool isStarted() const { return _started; }

		std::wstring getPath()
		{
			if (_thr)
			{
				_started = false;
				_thr->join();
				delete _thr;
				_thr = nullptr;
				return std::wstring(_path);
			}
		}
	};
	class SystemData
	{
	public:
		static const HWND hwnd;

		static void copyToClipboard(const std::string& str)
		{
			const size_t len = str.size() + 1;
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
			memcpy(GlobalLock(hMem), str.c_str(), len);
			GlobalUnlock(hMem);
			OpenClipboard(0);
			EmptyClipboard();
			SetClipboardData(CF_TEXT, hMem);
			CloseClipboard();
		}
		static std::string getFromClipboard()
		{
			OpenClipboard(0);
			HANDLE hData = GetClipboardData(CF_TEXT);
			char* p = static_cast<char*>(GlobalLock(hData));
			GlobalUnlock(hData);
			CloseClipboard();

			return std::string(p);
		}
		static inline void setConsoleVisibility(const bool boolean)
		{
			if (boolean)
				ShowWindow(::GetConsoleWindow(), SW_SHOW);
			else
				ShowWindow(::GetConsoleWindow(), SW_HIDE);
		}

		static std::wstring runOpenFileDialog(const wchar_t *filter = L"All files\0*.*\0", const wchar_t *defaultExtension = L"", const bool allowMultiple = false)
		{
			wchar_t path[4096] = { 0 };
			OPENFILENAME ofn = { sizeof(OPENFILENAME) };
			ofn.hwndOwner = hwnd;
			ofn.lpstrTitle = L"Open Atlas";
			ofn.lpstrFilter = filter;
			ofn.lpstrDefExt = L".txt";
			ofn.lpstrFile = path;
			ofn.nMaxFile = 4096;
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
			if (allowMultiple) ofn.Flags = OFN_ALLOWMULTISELECT;


			GetOpenFileNameW(&ofn);

			return std::wstring(path);
		}
	};
	const HWND SystemData::hwnd = GetConsoleWindow();
}

namespace img
{
	sf::Vector2f operator* (const sf::Vector2f first, const sf::Vector2f second) { return sf::Vector2f(first.x * second.x, first.y * second.y); }

	class ImageBox
	{
		bool _isSelected = false;
		sf::Image _im;
		sf::Texture _tx;
		sf::Sprite _sp;

	public:

		ImageBox(const std::string& sourcefile = "")
		{
			setImage(sourcefile);
		}

		ImageBox(const ImageBox& other) :
			_im(other._im),
			_tx(other._tx),
			_sp(_tx)
		{}

		void setImage(const std::string& sourcefile)
		{
			if (sourcefile.length() == 0 || !_im.loadFromFile(sourcefile))
				_im.loadFromFile("ui\\missing.png");
			_tx.create(_im.getSize().x, _im.getSize().y);
			_tx.update(_im);
			_sp.setTexture(_tx);
		}

		inline sf::Vector2f getPosition() const { return _sp.getPosition(); }

		inline void setSelect(const bool boolean) { _isSelected = boolean; }
		inline void setPosition(const sf::Vector2f& position) { _sp.setPosition(position);}
		inline void move(const sf::Vector2f& delta) { _sp.move(delta); }
		inline void setScale(const sf::Vector2f& scale) { _sp.setScale(scale); }
		inline void applyScale(const sf::Vector2f& scale) { _sp.setScale(_sp.getScale() * scale); }

		inline bool contains(const sf::Vector2f& coord) const { return _sp.getGlobalBounds().contains(coord); }
		inline bool intersects(const sf::FloatRect& rect) const { return rect.intersects(_sp.getGlobalBounds()); }

		inline const sf::Sprite& getDrawObject()
		{
			return _sp;
		}

		//help count instances in order to avoid leaks
		void* operator new(const std::size_t count)
		{
			++instanceCount;
			return ::operator new(count);
		}
		void operator delete(void *ptr)
		{
			--instanceCount;
			return ::operator delete(ptr);
		}

		static size_t instanceCount;
	};
	size_t ImageBox::instanceCount = 0;

	class ImageVector
	{
		std::vector<ImageBox*> _images;
		sf::Vector2f _position;
		sf::Vector2f _scale;

	public:
		ImageVector(const sf::Vector2f& position = sf::Vector2f(0.0, 0.0), const sf::Vector2f& scale = sf::Vector2f(1.0, 1.0)) :
			_position(position),
			_scale(scale)
		{}

		ImageVector(const ImageVector& other) :
			_position(other._position),
			_scale(other._scale)
		{
			for (size_t i = 0; i < other._images.size(); ++i)
				addImage(*new ImageBox(*other._images[i]));
		}

		ImageVector(ImageVector&& other) :
			_position(other._position),
			_scale(other._scale)
		{
			mergeWith(std::move(other));
		}

		~ImageVector()
		{
			for (size_t i = 0; i < _images.size(); ++i)
				delete _images[i];
		}

		inline void mergeWith(ImageVector&& other)
		{
			for (size_t i = 0; i < other._images.size(); ++i)
				addImage(*other._images[i]);
			other._images.clear();
		}

		inline void addImage(const std::string& sourcefile = "")
		{
			_images.push_back(new ImageBox(sourcefile));
			_images.back()->applyScale(_scale);
			if (_images.size() > 1)
				_images.back()->setPosition(sf::Vector2f(_images[_images.size() - 2]->getPosition() + sf::Vector2f(20.0, 20.0)));
			else
				_images.back()->setPosition(_position);
		}

		inline void addImage(ImageBox& image)
		{
			_images.push_back(&image);
			_images.back()->applyScale(_scale);
			if (_images.size() > 1)
				_images.back()->setPosition(_images[_images.size() - 2]->getPosition() + sf::Vector2f(20.0, 20.0));
			else
				_images.back()->setPosition(_position);
		}

		inline void setScale(const sf::Vector2f& scale)
		{
			_scale = scale;
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->setScale(scale);
		}
		inline void setPosition(const sf::Vector2f& position)
		{
			const sf::Vector2f delta = position - _position;
			_position = position;
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->move(delta);
		}
		inline void applyScale(const sf::Vector2f& scale)
		{
			_scale = _scale * scale;
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->applyScale(scale);
		}

		void deselectAll()
		{
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->setSelect(false);
		}
		void select(const sf::Vector2f& coord)
		{
			for(int i = _images.size() - 1; i >= 0; --i)
				if (_images[i]->contains(coord))
				{
					_images[i]->setSelect(true);
					return;
				}
				else
					_images[i]->setSelect(false);
		}
		void select(const sf::FloatRect& rect)
		{
			for (size_t i = 0; i < _images.size(); ++i)
				if (_images[i]->intersects(rect))
					_images[i]->setSelect(true);
				else
					_images[i]->setSelect(false);
		}

		inline void draw(sf::RenderWindow& window) const 
		{
			for (size_t i = 0; i < _images.size(); ++i)
				window.draw(_images[i]->getDrawObject());
		}
	};
}

namespace ui
{
	class Defined
	{
	public:
		static const std::wstring SupportedImageFormats;

		static const unsigned int CursorBlinkInterval = 500;

		static const std::string AlphaNumeric;
		static const std::string NonNegativeNumeric;
		static const std::string Numeric;

		static sf::Font DefaultFont;
		static sf::Text DefaultText;

		static sf::Texture Icons;

		static bool load()
		{
			DefaultFont.loadFromFile("texgyreadventor-regular.otf");
			DefaultText.setFont(DefaultFont);

			Icons.loadFromFile("iconsc.png");
		}
	};
	const std::wstring Defined::SupportedImageFormats = L"Image files (.bmp, .png, .tga, .jpg, .gif, .psd, .hdr, .pic)\0.bmp;.png;.tga;.jpg;.gif;.psd;.hdr;.pic\0";
	const std::string Defined::AlphaNumeric = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	const std::string Defined::NonNegativeNumeric = "0123456789";
	const std::string Defined::Numeric = "-0123456789";
	sf::Font Defined::DefaultFont;
	sf::Text Defined::DefaultText;
	sf::Texture Defined::Icons;

	using namespace img;

	enum ActionEvent
	{
		NONE,

		WINDOW_CLOSED,

		BUTTON_CLICKED_1,
		BUTTON_CLICKED_2,
		BUTTON_CLICKED_3,
		BUTTON_CLICKED_4,
		BUTTON_CLICKED_5,

		INPUTBOX_ENTER_1,
		INPUTBOX_ENTER_2,
		INPUTBOX_ENTER_3,

		TICKBOX_MODIFIED_1,
		TICKBOX_MODIFIED_2,
		TICKBOX_MODIFIED_3,
	};

	class Background
	{
		sf::RectangleShape _back;

	public:
		Background(const sf::Vector2f& size, const sf::Vector2f& position, const sf::Color& color) :
			_back(size)
		{
			_back.setPosition(position);
			_back.setFillColor(color);
		}

		inline void setFillColor(sf::Color& color) { _back.setFillColor(color); }

		inline void draw(sf::RenderWindow& window)
		{
			window.draw(_back);
		}
	};

	enum TextAlignment
	{
		Center = 1,
		Left = 2,
		Right = 3,
	};
	class TextBox
	{
		sf::Text _text;

	public:
		TextBox(
			const sf::Vector2f& position,
			const unsigned short int textSize,
			const std::string& text,
			const sf::Font& font = Defined::DefaultFont,
			const sf::Color& textColor = sf::Color(255, 255, 255, 255)
		) :
			_text(text, font, textSize)
		{
			_text.setFillColor(textColor);
			_text.setPosition(static_cast<int>(position.x), static_cast<int>(position.y));
		}

		inline void draw(sf::RenderWindow& window) const
		{
			window.draw(_text);
		}
	};

	class Button
	{
		bool _hovered = false;

	protected:
		const ActionEvent _returnEvent;
		const sf::Color _originalColor;
		sf::RectangleShape _back;

		inline virtual void _checkHovering(const sf::Vector2f& point)
		{
			if (!_hovered && _back.getGlobalBounds().contains(point))
			{
				_back.setFillColor(sf::Color(_originalColor.r + 50, _originalColor.g + 50, _originalColor.b + 50, _originalColor.a));
				_back.setOutlineThickness(-1.0);
				_hovered = true;
			}
			else if (_hovered && !_back.getGlobalBounds().contains(point))
			{
				_back.setFillColor(_originalColor);
				_back.setOutlineThickness(0.0);
				_hovered = false;
			}
		}

	public:
		Button(const sf::Vector2f& size, const sf::Vector2f& position, const ActionEvent returnEvent, const sf::Color& fillColor = sf::Color(0, 0, 0, 0)) :
			_back(size),
			_originalColor(fillColor),
			_returnEvent(returnEvent)
		{
			_back.setPosition(position);
			_back.setFillColor(fillColor);
			_back.setOutlineColor(sf::Color(fillColor.r + 100, fillColor.g + 100, fillColor.b + 100, fillColor.a));
		}

		inline bool contains(const sf::Vector2f& point) { return _back.getGlobalBounds().contains(point); }

		inline const ActionEvent click() const { return _returnEvent; }

		virtual void draw(sf::RenderWindow& window, const sf::Vector2f& point)
		{
			_checkHovering(point);
			window.draw(_back);
		}
	};
	class TickBox : public Button
	{
		bool _isTicked = false;
		sf::Sprite _tickSprite;

	public:
		TickBox(const sf::Vector2f& size, const sf::Vector2f& position, const ActionEvent returnEvent, const bool isTicked = false, const sf::Color& fillColor = sf::Color(0, 0, 0, 0)) :
			Button(size, position, returnEvent, fillColor),
			_tickSprite(Defined::Icons),
			_isTicked(isTicked)
		{
			_tickSprite.setColor(sf::Color::Green);
			_tickSprite.setPosition(position);

			_tickSprite.setScale(0.01, 0.01);
		}

		void tick()
		{ 
			_isTicked = !_isTicked;
		}
		inline bool isTicked() const { return _isTicked; }

		void draw(sf::RenderWindow& window, const sf::Vector2f& point) override final
		{
			_checkHovering(point);
			window.draw(_back);
			if(_isTicked)
				window.draw(_tickSprite);
		}
	};
	class TextButton : public Button
	{
	protected:
		sf::Text _text;

	public:
		TextButton(
			const sf::Vector2f& size,
			const sf::Vector2f& position,
			const unsigned short int textSize,
			const std::string& text,
			const ActionEvent returnEvent,
			const sf::Font& font = Defined::DefaultFont,
			const TextAlignment alignment = TextAlignment::Center,
			const sf::Color& fillColor = sf::Color(0, 0, 0, 0),
			const sf::Color& textColor = sf::Color(255, 255, 255, 255)
		) :
			Button(size, position, returnEvent, fillColor),
			_text(text, font, textSize)
		{
			_text.setFillColor(textColor);
			
			if (_back.getGlobalBounds().width > 0)
			{
				size_t textLen = text.size();
				while (_text.getGlobalBounds().width + 10 >= _back.getGlobalBounds().width)
					_text.setString(text.substr(0, --textLen));

				if (textLen != text.size())
					_text.setString(text.substr(0, textLen - 3) + "...");
			}

			if (alignment == TextAlignment::Center)
				_text.setPosition(static_cast<int>(position.x + (size.x - _text.getGlobalBounds().width) / 2.0), static_cast<int>(position.y + (size.y - _text.getGlobalBounds().height) / 2.0));
			else if (alignment == TextAlignment::Left)
				_text.setPosition(position.x + 5, static_cast<int>(position.y + (size.y - _text.getGlobalBounds().height) / 2.0));
			else if (alignment == TextAlignment::Right)
				_text.setPosition(position.x + size.x - _text.getGlobalBounds().width - 5, static_cast<int>(position.y + (size.y - _text.getGlobalBounds().height) / 2.0));
		}

		void draw(sf::RenderWindow& window, const sf::Vector2f& point) override
		{
			_checkHovering(point);
			window.draw(_back);
			window.draw(_text);
		}
	};
	class SelectableTextButton : public TextButton
	{
	protected:
		bool _isSelected = false;

	public:
		SelectableTextButton(
			const sf::Vector2f& size,
			const sf::Vector2f& position,
			const unsigned short int textSize,
			const std::string& text,
			const ActionEvent returnEvent,
			const sf::Font& font = Defined::DefaultFont,
			const TextAlignment alignment = TextAlignment::Center,
			const sf::Color& fillColor = sf::Color(0, 0, 0, 0),
			const sf::Color& textColor = sf::Color(255, 255, 255, 255)
		) :
			TextButton(size, position, textSize, text, returnEvent, font, alignment, fillColor, textColor)
		{}

		inline bool isSelected() const { return _isSelected; }
		inline void setSelected(const bool boolean) { _isSelected = boolean; }

		void draw(sf::RenderWindow &window, const sf::Vector2f& point) override final
		{
			if (!_isSelected)
				_checkHovering(point);

			window.draw(_back);
			window.draw(_text);
		}

	};
	class InputBox : public SelectableTextButton
	{
		unsigned short int _blink = 0;
		const std::string& _allowedSymbols;
		const sf::Color _textColor;
		sf::RectangleShape _cursor;

	protected:
		int _cursorPos = 0;
		std::string _inputString = "";
		const std::string _defaultText;

		void _checkEmptyString()
		{
			if (_inputString.length())
			{
				_text.setString(_inputString);
				_text.setFillColor(_textColor);
				_text.setStyle(sf::Text::Regular);
			}
			else
			{
				_text.setString(_defaultText);
				_text.setFillColor(sf::Color(_textColor.r / 2, _textColor.g / 2, _textColor.b / 2, _textColor.a));
				_text.setStyle(sf::Text::Italic);
				_cursor.setPosition(_back.getPosition().x + 3, _back.getPosition().y + (_back.getSize().y - 14) / 2.0);
			}
		}

	public:
		InputBox(
			const sf::Vector2f& size,
			const sf::Vector2f& position,
			const unsigned short int textSize,
			const std::string& defaultText,
			const ActionEvent returnEvent,
			const sf::Font& font = Defined::DefaultFont,
			const std::string& allowedSymbols = Defined::AlphaNumeric,
			const sf::Color& fillColor = sf::Color(0, 0, 0, 0),
			const sf::Color& textColor = sf::Color(255, 255, 255, 255)
		) :
			SelectableTextButton(size, position, textSize, defaultText, returnEvent, font, TextAlignment::Left, fillColor, textColor),
			_defaultText(defaultText),
			_textColor(textColor),
			_cursor(sf::Vector2f(1.0, textSize + 8.0)),
			_allowedSymbols(allowedSymbols)
		{
			_text.setFillColor(sf::Color(textColor.r / 2, textColor.g / 2, textColor.b / 2, textColor.a));
			_text.setStyle(sf::Text::Italic);
			_cursor.setPosition(sf::Vector2f(_text.getPosition().x, _text.getPosition().y - 5));
		}

		void moveCursor(const int delta)
		{
			if (_cursorPos + delta >= 0 && _cursorPos + delta <= _inputString.size())
			{
				_cursorPos += delta;
				const sf::Text temp(_inputString.substr(0, _cursorPos), *_text.getFont(), _text.getCharacterSize());
				_cursor.setPosition(_back.getPosition().x + temp.getGlobalBounds().width + 5, _cursor.getPosition().y);
				_blink = 5;
			}
		}
		virtual const ActionEvent modifyString(const char c)
		{
			if (c == 13)
			{
				_isSelected = false;
				return _returnEvent;
			}

			if (c == 8 && _cursorPos > 0)
			{
				_inputString.erase(_cursorPos - 1, 1);
				moveCursor(-1);
			}
			else if (_allowedSymbols.find(c) != std::string::npos)
			{
				_inputString.insert(_inputString.begin() + _cursorPos, c);
				moveCursor(1);
			}
			_checkEmptyString();

			return ActionEvent::NONE;
		}
		virtual void insertString(const std::string& str)
		{
			for (size_t i = 0; i < str.size(); ++i)
				if (_allowedSymbols.find(str[i]) == std::string::npos)
					return;

			_inputString.insert(_cursorPos, str);
			moveCursor(str.size());
			_checkEmptyString();
		}

		inline const std::string getContent() const { return _inputString.size() ? _inputString : _defaultText; }

		void draw(sf::RenderWindow &window, const sf::Vector2f& point, const bool drawCursor = true)
		{
			if(!_isSelected)
				_checkHovering(point);

			window.draw(_back);
			window.draw(_text);

			if (_isSelected && drawCursor)
				window.draw(_cursor);
		}
	};
	class IntegerInputBox : public InputBox
	{
		const int _minValue, _maxValue;

		inline void _setToMin()
		{
			_inputString = std::to_string(_minValue);
			_text.setString(_inputString.size() ? _inputString : _defaultText);
			moveCursor(_inputString.size() - _cursorPos);
		}
		inline void _setToMax()
		{
			_inputString = std::to_string(_maxValue);
			_text.setString(_inputString.size() ? _inputString : _defaultText);
			moveCursor(_inputString.size() - _cursorPos);
		}

	public:
		IntegerInputBox(
			const sf::Vector2f& size,
			const sf::Vector2f& position,
			const unsigned short int textSize,
			const int minValue,
			const int maxValue,
			const ActionEvent returnEvent,
			const sf::Font& font = Defined::DefaultFont,
			const sf::Color& fillColor = sf::Color(0, 0, 0, 0),
			const sf::Color& textColor = sf::Color(255, 255, 255, 255)
		) :
			InputBox(size, position, textSize, std::to_string((minValue + maxValue) / 2), returnEvent, font, "", fillColor, textColor),
			_minValue(minValue),
			_maxValue(maxValue)
		{}

		const ActionEvent modifyString(const char c) override final 
		{
			if (c == 13)
			{
				_isSelected = false;
				return _returnEvent;
			}

			if (c == 8 && _cursorPos > 0)
			{
				_inputString.erase(_cursorPos - 1, 1);
				moveCursor(-1);
			}
			else if ((c == '-' && _cursorPos == 0) || ((_inputString.size() == 0 || _inputString[_cursorPos] != '-') && isdigit(c)))
			{
				_inputString.insert(_inputString.begin() + _cursorPos, c);

				const int temp = getValue();
				if (temp < _minValue)
				{
					_setToMin();
					return ActionEvent::NONE;
				}
				else if (temp > _maxValue)
				{
					_setToMax();
					return ActionEvent::NONE;
				}

				moveCursor(1);
			}
			_checkEmptyString();

			return ActionEvent::NONE;
		}
		void insertString(const std::string& str) override final 
		{
			if(!util::isInteger(str))
				return;

			const int temp = std::stoi(str);
			if (temp < _minValue)
			{
				_setToMin();
				return;
			}
			else if (temp > _maxValue)
			{
				_setToMax();
				return;
			}

			_inputString = str;
			moveCursor(str.size() - _cursorPos);
			_checkEmptyString();
		}

		int getValue() const { return _inputString.size() ? ((_inputString[0] == '-' && _inputString.size() == 1) ? 0 : std::stoi(_inputString)) : (_minValue + _maxValue) / 2; }
		void modifyValue(const int delta)
		{
			const int temp = getValue() + delta;
			if (temp < _minValue)
			{
				_setToMin();
				return;
			}
			if (temp > _maxValue)
			{
				_setToMax();
				return;
			}

			_inputString = std::to_string(temp);
			_checkEmptyString();
			moveCursor(_inputString.size() - _cursorPos);
		}
	};
	class ColorBox
	{
		const IntegerInputBox const *_rIn, *_gIn, *_bIn;
		sf::CircleShape _back;

	public:
		ColorBox(const float diameter, const sf::Vector2f& position, const IntegerInputBox *red = nullptr, const IntegerInputBox *green = nullptr, const IntegerInputBox *blue = nullptr) :
			_rIn(red),
			_gIn(green),
			_bIn(blue),
			_back(diameter / 2.0, 6)
		{
			_back.setPosition(position);
			_back.setOutlineColor(sf::Color(120, 120, 120, 255));
			_back.setOutlineThickness(-1);
		}

		inline sf::Color _getColor() const
		{
			return sf::Color(_rIn ? _rIn->getValue() : 0, _gIn ? _gIn->getValue() : 0, _bIn ? _bIn->getValue() : 0, 255);
		}

		void draw(sf::RenderWindow& window)
		{
			_back.setFillColor(_getColor());
			window.draw(_back);
		}

	};


	template<class T> class Vector
	{
		std::vector<T*> _content;

	public:
		Vector() {}
		Vector(const Vector& vector) = delete;

		~Vector()
		{
			for (size_t i = 0; i < _content.size(); ++i)
				delete _content[i];
		}

		inline void add(T* element) { _content.push_back(element); }
		inline void remove(const size_t idx) { delete _content[idx]; _content.erase(_content.begin() + idx); }

		void drawAll(sf::RenderWindow& window)
		{
			for (size_t i = 0; i < _content.size(); ++i)
				_content[i]->draw(window);
		}
	};
	template<> class Vector <Button>
	{
		std::vector<Button*> _content;

	public:
		Vector() {}
		Vector(const Vector& vector) = delete;

		~Vector()
		{
			for (size_t i = 0; i < _content.size(); ++i)
				delete _content[i];
		}

		inline void add(Button* element) { _content.push_back(element); }
		inline void remove(const size_t idx) { delete _content[idx]; _content.erase(_content.begin() + idx); }

		inline const Button* contains(const sf::Vector2f& point)
		{
			for (int i = _content.size() - 1; i >= 0; --i)
				if (_content[i]->contains(point))
					return _content[i];
			return nullptr;
		}

		void drawAll(sf::RenderWindow& window, const sf::Vector2f& point)
		{
			for (size_t i = 0; i < _content.size(); ++i)
				_content[i]->draw(window, point);
		}
	};

	class EditorTab
	{
		ImageVector _content;
		std::string _name;
		sf::RectangleShape _background;

	public:
		EditorTab(const sf::Vector2f& size, const sf::Vector2f& position, const std::string& name = "New Tab") :
			_background(size),
			_name(name)
		{
			_background.setFillColor(sf::Color(150, 150, 150, 255));
		}

		inline void draw(sf::RenderWindow& window) const 
		{
			window.draw(_background);
			_content.draw(window);
		}
	};

	class EditorUI
	{
		sf::RenderWindow& _window;
		std::vector<EditorTab> _tabs;
		size_t _frontTab = 0;

	public:
		EditorUI(sf::RenderWindow& window) :
			_window(window)
		{}

		void run()
		{
			while (_window.isOpen())
			{
				if (_tabs.size() == 0) // start menu
				{
					//_handleEvents();
					//_display();
				}
			}

		}
	};

	class StartMenuUI
	{
		enum StateTag
		{
			NONE,
			NEW_RGBA_ATLAS,
			NEW_RGB_ATLAS,
			OPEN_ATLAS
		};

		StateTag _state;
		sf::RenderWindow& _window;
		sf::Clock _clock;
		Background _back1, _back2;
		SelectableTextButton _newButton, _openButton;
		TextButton _continueButton;
		Vector<Button> _buttons;
		TextBox _recentsTextBox, _enterNameTextBox;
		InputBox _nameBox;
		TickBox _tickBox;
		IntegerInputBox _intBox;
		ColorBox _colBox;
		sf::Mouse _mouse;
		os::OpenFileDialog _openFileDialog;


		ActionEvent _handleDefaultEvents()
		{
			sf::Event event;
			while (_window.pollEvent(event))
			{
				switch (event.type)
				{
				case sf::Event::MouseButtonReleased:
				{
					const sf::Vector2f mousePos = static_cast<sf::Vector2f>(_mouse.getPosition(_window));

					//click buttons
					const Button* button = _buttons.contains(mousePos);
					if (button)
					{
						button->click();
					}

					if (_newButton.contains(mousePos))
					{
						if (_openFileDialog.isStarted())
							_openFileDialog.kill();

						_newButton.setSelected(true);
						_state = StateTag::NEW_RGBA_ATLAS;
					}
					else _newButton.setSelected(false);
					
					if (_openButton.contains(mousePos))
					{
						_openButton.setSelected(true);
						_openFileDialog.start();
					}
					else _openButton.setSelected(false);
				}
				break;

				case sf::Event::Closed:
					return ActionEvent::WINDOW_CLOSED;
				}
			}
		}
		ActionEvent _handleNewAtlasEvents()
		{
			sf::Event event;
			while (_window.pollEvent(event))
			{
				switch (event.type)
				{
				case sf::Event::MouseButtonReleased:
				{
					const sf::Vector2f mousePos = static_cast<sf::Vector2f>(_mouse.getPosition(_window));
					_nameBox.setSelected(_nameBox.contains(mousePos));

					if (_tickBox.contains(mousePos)) _tickBox.tick();

					if (_openButton.contains(mousePos))
					{
						_openButton.setSelected(true);
						_newButton.setSelected(false);
						_state = StateTag::NONE;
						//_displayStartMenu();
						_openFileDialog.start();
						//os::SystemData::runOpenFileDialog(Defined::SupportedImageFormats.c_str());
					}
				}
				break;

				case sf::Event::KeyPressed:
				{
					if (event.key.code == sf::Keyboard::Left)
					{
						if (_nameBox.isSelected())
							_nameBox.moveCursor(-1);
					}
					else if (event.key.code == sf::Keyboard::Right)
					{
						if (_nameBox.isSelected())
							_nameBox.moveCursor(1);
					}
					else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && sf::Keyboard::isKeyPressed(sf::Keyboard::V))
					{
						if (_nameBox.isSelected())
							_nameBox.insertString(os::SystemData::getFromClipboard());
					}
				}
				break;

				case sf::Event::TextEntered:
				{
					if (_nameBox.isSelected())
						_nameBox.modifyString(event.text.unicode);
				}
				break;

				case sf::Event::Closed:
					return ActionEvent::WINDOW_CLOSED;
				}
			}
		}

		ActionEvent _handleEvents()
		{
			switch (_state)
			{
			case StateTag::NONE:
				return _handleDefaultEvents();
			case StateTag::NEW_RGBA_ATLAS:
				return _handleNewAtlasEvents();
			}
		}


		void _displayStartMenu()
		{
			_window.clear();
			_back1.draw(_window);

			_newButton.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));
			_openButton.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));
			_recentsTextBox.draw(_window);
			_buttons.drawAll(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));


			if (_state == StateTag::NEW_RGBA_ATLAS)
			{
				_back2.draw(_window);
				_enterNameTextBox.draw(_window);
				_nameBox.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)), (_clock.getElapsedTime().asMilliseconds() / Defined::CursorBlinkInterval) & 1);
				_tickBox.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));
				_continueButton.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));
			}

		
			
			//_intBox.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)), (_clock.getElapsedTime().asMilliseconds() / Defined::CursorBlinkInterval) & 1);
			//_colBox.draw(_window);

			_window.display();
		}

	public:
		StartMenuUI(sf::RenderWindow& window) :
			_window(window),
			_back1(sf::Vector2f(300.0, window.getSize().y), sf::Vector2f(0.0, 0.0), sf::Color(sf::Color(20, 20, 20, 255))),
			_back2(sf::Vector2f(window.getSize().x - 300, window.getSize().y), sf::Vector2f(300.0, 0.0), sf::Color(sf::Color(40, 40, 40, 255))),
			_recentsTextBox(sf::Vector2f(50.0, 200.0), 16, "Recent files:", Defined::DefaultFont, sf::Color(11, 129, 133, 255)),
			_newButton(sf::Vector2f(200.0, 40.0), sf::Vector2f(50.0, 50.0), 18, "Create New Atlas", ActionEvent::BUTTON_CLICKED_1, Defined::DefaultFont, TextAlignment::Left, sf::Color(40, 40, 40, 255)),
			_openButton(sf::Vector2f(200.0, 40.0), sf::Vector2f(50.0, 120.0), 18, "Open Existing Atlas", ActionEvent::BUTTON_CLICKED_2, Defined::DefaultFont, TextAlignment::Left, sf::Color(40, 40, 40, 255)),
			_enterNameTextBox(sf::Vector2f(350.0, 50.0), 14, "New Atlas Name: ", Defined::DefaultFont, sf::Color(180, 180, 180, 255)),
			_nameBox(sf::Vector2f(400.0, 50.0), sf::Vector2f(350.0, 70.0), 16, "NewAtlas", ActionEvent::INPUTBOX_ENTER_1, Defined::DefaultFont, Defined::AlphaNumeric, sf::Color(20, 20, 20, 255)),
			_tickBox(sf::Vector2f(20.0, 20.0), sf::Vector2f(350.0, 130.0), ActionEvent::TICKBOX_MODIFIED_1, false, sf::Color(20, 20, 20, 255)),
			_continueButton(sf::Vector2f(100.0, 40.0), sf::Vector2f(1050.0, 720.0), 18, "Continue", ActionEvent::BUTTON_CLICKED_3, Defined::DefaultFont, TextAlignment::Center, sf::Color(11, 129, 133, 255)),
			_intBox(sf::Vector2f(60.0, 40.0), sf::Vector2f(300.0, 170.0), 16, 0, 255, ActionEvent::INPUTBOX_ENTER_2, Defined::DefaultFont, sf::Color(20, 20, 20, 255), sf::Color(255, 255, 0, 255)),
			_colBox(50.0, sf::Vector2f(400.0, 170.0), &_intBox),
			_openFileDialog(os::SystemData::hwnd)
		{
			std::ifstream recentIn("recents.txt");
			if (recentIn.good())
			{
				std::string tempStr;
				unsigned int cnt = 0;
				while (cnt < 10 && getline(recentIn, tempStr))
				{
					_buttons.add(new TextButton(
						sf::Vector2f(200.0, 30.0), 
						sf::Vector2f(50.0, 220 + cnt * 35.0), 16, 
						tempStr, 
						ActionEvent::BUTTON_CLICKED_3, 
						Defined::DefaultFont,
						TextAlignment::Left,
						sf::Color(20, 20, 20, 255),
						sf::Color(255 - cnt * 20, 255 - cnt * 20, 255 - cnt * 20, 255)));
					++cnt;
				}
			}
			recentIn.close();

		}

		ActionEvent run()
		{
			while (true)
			{
				if (_openFileDialog.isStarted() && _openFileDialog.isReady())
					std::wcout << _openFileDialog.getPath();

				if(_handleEvents() == ActionEvent::WINDOW_CLOSED)
					return ActionEvent::WINDOW_CLOSED;
				_displayStartMenu();
			}
			return ActionEvent::NONE;
		}
	};

	class TestMenuUI
	{
		sf::RenderWindow& _window;
		sf::Clock _clock;
		Vector<Button> _buttons;
		Vector<TextBox> _textBoxes;
		InputBox _nameBox;
		TickBox _tickBox;
		IntegerInputBox _intBox;
		ColorBox _colBox;
		sf::Mouse _mouse;



		ActionEvent _handleStartMenuEvents()
		{
			sf::Event event;
			while (_window.pollEvent(event))
			{
				switch (event.type)
				{
				case sf::Event::MouseButtonReleased:
				{
					const sf::Vector2f mousePos = static_cast<sf::Vector2f>(_mouse.getPosition(_window));

					//click buttons
					const Button* button = _buttons.contains(mousePos);
					if (button) button->click();

					//select/deselect namebox
					_nameBox.setSelected(_nameBox.contains(mousePos));

					//tick/untick boxes
					if (_tickBox.contains(mousePos)) _tickBox.tick();

					_intBox.setSelected(_intBox.contains(mousePos));
				}
				break;

				case sf::Event::KeyPressed:
				{
					if (event.key.code == sf::Keyboard::Left)
					{
						if (_nameBox.isSelected())
							_nameBox.moveCursor(-1);
						else if (_intBox.isSelected())
							_intBox.moveCursor(-1);
					}
					else if (event.key.code == sf::Keyboard::Right)
					{
						if (_nameBox.isSelected())
							_nameBox.moveCursor(1);
						else if (_intBox.isSelected())
							_intBox.moveCursor(1);
					}
					else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && sf::Keyboard::isKeyPressed(sf::Keyboard::V))
					{
						std::cout << "Ctrl-V\n";
						if (_nameBox.isSelected())
							_nameBox.insertString(os::SystemData::getFromClipboard());
						else if (_intBox.isSelected())
							_intBox.insertString(os::SystemData::getFromClipboard());
					}
				}
				break;

				case sf::Event::TextEntered:
				{
					if (_nameBox.isSelected())
						_nameBox.modifyString(event.text.unicode);
					else if (_intBox.isSelected())
						_intBox.modifyString(event.text.unicode);
				}
				break;

				case sf::Event::MouseWheelMoved:
				{
					if (_intBox.isSelected())
						_intBox.modifyValue(event.mouseWheel.delta);
				}
				break;

				case sf::Event::Closed:
					return ActionEvent::WINDOW_CLOSED;
				}
			}
		}
		void _displayStartMenu()
		{
			_window.clear();
			_textBoxes.drawAll(_window);
			_buttons.drawAll(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));
			_nameBox.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)), (_clock.getElapsedTime().asMilliseconds() / Defined::CursorBlinkInterval) & 1);
			_tickBox.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));
			_intBox.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)), (_clock.getElapsedTime().asMilliseconds() / Defined::CursorBlinkInterval) & 1);
			_colBox.draw(_window);
			_window.display();
		}

	public:
		TestMenuUI(sf::RenderWindow& window) :
			_window(window),
			_nameBox(sf::Vector2f(400.0, 50.0), sf::Vector2f(300.0, 50.0), 16, "NewAtlas", ActionEvent::INPUTBOX_ENTER_1, Defined::DefaultFont, Defined::AlphaNumeric, sf::Color(20, 20, 20, 255)),
			_tickBox(sf::Vector2f(20.0, 20.0), sf::Vector2f(300.0, 130.0), ActionEvent::TICKBOX_MODIFIED_1, false, sf::Color(60, 60, 60, 255)),
			_intBox(sf::Vector2f(60.0, 40.0), sf::Vector2f(300.0, 170.0), 16, 0, 255, ActionEvent::INPUTBOX_ENTER_2, Defined::DefaultFont, sf::Color(20, 20, 20, 255), sf::Color(255, 255, 0, 255)),
			_colBox(50.0, sf::Vector2f(400.0, 170.0), &_intBox)
		{
			_buttons.add(new TextButton(sf::Vector2f(200.0, 50.0), sf::Vector2f(50.0, 50.0), 16, "Create New Atlas", ActionEvent::BUTTON_CLICKED_1, Defined::DefaultFont, TextAlignment::Left, sf::Color(20, 20, 20, 255)));
			_buttons.add(new TextButton(sf::Vector2f(200.0, 50.0), sf::Vector2f(50.0, 120.0), 16, "Open Existing Atlas", ActionEvent::BUTTON_CLICKED_2, Defined::DefaultFont, TextAlignment::Left, sf::Color(20, 20, 20, 255)));

			_textBoxes.add(new TextBox(sf::Vector2f(50.0, 200.0), 16, "Recent files:", Defined::DefaultFont, sf::Color(255, 255, 0, 255)));
			std::ifstream recentIn("recents.txt");
			if (recentIn.good())
			{
				std::string tempStr;
				unsigned int cnt = 0;
				while (cnt < 10 && getline(recentIn, tempStr))
				{
					_buttons.add(new TextButton(sf::Vector2f(200.0, 30.0), sf::Vector2f(50.0, 220 + cnt * 35.0), 16, tempStr, ActionEvent::BUTTON_CLICKED_5, Defined::DefaultFont, TextAlignment::Left, sf::Color(20, 20, 20, 255)));
					++cnt;
				}
			}
			recentIn.close();

		}

		ActionEvent run()
		{
			while (true)
			{
				if (_handleStartMenuEvents() == ActionEvent::WINDOW_CLOSED)
					return ActionEvent::WINDOW_CLOSED;
				_displayStartMenu();
			}
			return ActionEvent::NONE;
		}
	};

	class UIDriver
	{
		sf::RenderWindow _window;
		StartMenuUI _startMenu;
		EditorUI _editor;

	public:
		UIDriver(const sf::VideoMode& videoMode) :
			_window(videoMode, "< Aspershine >"),
			_startMenu(_window),
			_editor(_window)
		{
			_window.setFramerateLimit(60);

			if(_startMenu.run() == ActionEvent::WINDOW_CLOSED)
				_window.close();
		}


	};

}

int main()
{
	ui::Defined::load();
	ui::UIDriver driver(sf::VideoMode(1200, 800));
	std::cout << img::ImageBox::instanceCount;
	getchar();

	return 0;
}
