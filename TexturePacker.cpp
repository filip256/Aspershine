#include <SFML/Graphics.hpp>
#include <iostream>
#include "dirent.h"
#include <filesystem>

namespace os
{
	class SystemData
	{
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
	};

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
			for(size_t i = _images.size() - 1; i >= 0; --i)
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
		static const std::string AlphaNumeric;
		static const std::string NonNegativeNumeric;
		static const std::string Numeric;

		static sf::Font DefaultFont;
		static sf::Text DefaultText;

		static bool load()
		{
			DefaultFont.loadFromFile("default.ttf");
			DefaultText.setFont(DefaultFont);
		}
	};
	const std::string Defined::AlphaNumeric = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	const std::string Defined::NonNegativeNumeric = "0123456789";
	const std::string Defined::Numeric = "-0123456789";
	sf::Font Defined::DefaultFont;
	sf::Text Defined::DefaultText;

	using namespace img;



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
		sf::RectangleShape _back;

		inline virtual void _checkHovering(const sf::Vector2f& point)
		{
			if (!_hovered && _back.getGlobalBounds().contains(point))
			{
				const sf::Color color = _back.getFillColor();
				_back.setFillColor(sf::Color(color.r + 50, color.g + 50, color.b + 50, color.a));
				_back.setOutlineThickness(-1.0);
				_hovered = true;
			}
			else if (_hovered && !_back.getGlobalBounds().contains(point))
			{
				const sf::Color color = _back.getFillColor();
				_back.setFillColor(sf::Color(color.r - 50, color.g - 50, color.b - 50, color.a));
				_back.setOutlineThickness(0.0);
				_hovered = false;
			}
		}

	public:
		Button(const sf::Vector2f& size, const sf::Vector2f& position, const sf::Color& fillColor = sf::Color(0, 0, 0, 0)) :
			_back(size)
		{
			_back.setPosition(position);
			_back.setFillColor(fillColor);
			_back.setOutlineColor(sf::Color(fillColor.r + 100, fillColor.g + 100, fillColor.b + 100, fillColor.a));
		}

		inline bool contains(const sf::Vector2f& point) { return _back.getGlobalBounds().contains(point); }

		virtual inline void draw(sf::RenderWindow& window, const sf::Vector2f& point)
		{
			_checkHovering(point);
			window.draw(_back);
		}
	};
	class TickBox : public Button
	{
		bool _isTicked = false;
		const sf::Color _originalColor;

	public:
		TickBox(const sf::Vector2f& size, const sf::Vector2f& position, const bool isTicked = false, const sf::Color& fillColor = sf::Color(0, 0, 0, 0)) :
			Button(size, position, fillColor),
			_originalColor(fillColor),
			_isTicked(isTicked)
		{
			if (isTicked)
				_back.setFillColor(sf::Color(75, 255, 75, 255));
		}

		void tick()
		{ 
			_isTicked != _isTicked;
			_back.setFillColor(_isTicked ? sf::Color(75, 255, 75, 255) : _originalColor);
		}
		inline bool isTicked() const { return _isTicked; }
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
			const sf::Font& font = Defined::DefaultFont,
			const TextAlignment alignment = TextAlignment::Center,
			const sf::Color& fillColor = sf::Color(0, 0, 0, 0),
			const sf::Color& textColor = sf::Color(255, 255, 255, 255)
		) :
			Button(size, position, fillColor),
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

		virtual inline void draw(sf::RenderWindow& window, const sf::Vector2f& point) override
		{
			_checkHovering(point);
			window.draw(_back);
			window.draw(_text);
		}
	};
	class InputBox : TextButton
	{
		unsigned short int _blink = 0;
		const std::string _defaultText;
		const sf::Color _textColor;
		sf::RectangleShape _cursor;

	protected:
		bool _isSelected = false;
		int _cursorPos = 0;
		const std::string& _allowedSymbols;
		std::string _inputString = "";

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
				_text.setFillColor(sf::Color(_textColor.r - 30, _textColor.g - 30, _textColor.b - 30, _textColor.a));
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
			const sf::Font& font = Defined::DefaultFont,
			const std::string& allowedSymbols = Defined::AlphaNumeric,
			const sf::Color& fillColor = sf::Color(0, 0, 0, 0),
			const sf::Color& textColor = sf::Color(255, 255, 255, 255)
		) :
			TextButton(size, position, textSize, defaultText, font, TextAlignment::Left, fillColor, textColor),
			_defaultText(defaultText),
			_textColor(textColor),
			_cursor(sf::Vector2f(2.0, 24.0)),
			_allowedSymbols(allowedSymbols)
		{
			_text.setFillColor(sf::Color(textColor.r - 30, textColor.g - 30, textColor.b - 30, textColor.a));
		}

		inline void setSelected(const bool boolean) { _isSelected = boolean; }
		void moveCursor(const int delta)
		{
			if (_cursorPos + delta >= 0 && _cursorPos + delta <= _inputString.length())
			{
				_cursorPos += delta;
				_cursor.setPosition(_back.getPosition().x + _cursorPos * 14, _cursor.getPosition().y);
				_blink = 5;
			}
		}
		virtual bool modifyString(const char c)
		{
			if (c == 13)
			{
				_isSelected = false;
				return true;
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

			return false;
		}
		virtual void insertString(const std::string& str)
		{
			for (size_t i = 0; i < str.size(); ++i)
				if (_allowedSymbols.find(str[i]) == std::string::npos)
					return;

			_inputString.insert(_cursorPos, str);
			moveCursor(str.size());
		}

		void draw(sf::RenderWindow &window, const sf::Vector2f& point) override
		{
			if(!_isSelected)
				_checkHovering(point);

			window.draw(_back);
			window.draw(_text);

			if (_isSelected)
			{
				if (_blink < 15)
					window.draw(_cursor);

				if (_blink < 30)
					++_blink;
				else
					_blink = 0;
			}
		}
	};
	class IntegerInputBox : public InputBox
	{
		const int _minValue, _maxValue;

		inline void _setToMin()
		{
			_inputString = std::to_string(_minValue);
			moveCursor(-_cursorPos);
		}
		inline void _setToMax()
		{
			_inputString = std::to_string(_maxValue);
			moveCursor(-_cursorPos);
		}

	public:
		IntegerInputBox(
			const sf::Vector2f& size,
			const sf::Vector2f& position,
			const unsigned short int textSize,
			const int minValue,
			const int maxValue,
			const sf::Font& font = Defined::DefaultFont,
			const sf::Color& fillColor = sf::Color(0, 0, 0, 0),
			const sf::Color& textColor = sf::Color(255, 255, 255, 255)
		) :
			InputBox(size, position, textSize, std::to_string(_minValue), font, Defined::Numeric, fillColor, textColor),
			_minValue(minValue),
			_maxValue(maxValue)
		{}

		bool modifyString(const char c) override
		{
			if (c == 13)
			{
				_isSelected = false;
				return true;
			}

			if (c == 8 && _cursorPos > 0)
			{
				_inputString.erase(_cursorPos - 1, 1);
				moveCursor(-1);
			}
			else if (_allowedSymbols.find(c) != std::string::npos)
			{
				_inputString.insert(_inputString.begin() + _cursorPos, c);

				const int temp = getValue();
				if (temp < _minValue)
				{
					_setToMin();
					return false;
				}
				else if (temp > _maxValue)
				{
					_setToMax();
					return false;
				}

				moveCursor(1);
			}
			_checkEmptyString();

			return false;
		}
		void insertString(const std::string& str) override
		{
			for (size_t i = 0; i < str.size(); ++i)
				if (_allowedSymbols.find(str[i]) == std::string::npos)
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
		}

		int getValue() const { return _inputString.size() ? std::stoi(_inputString) : _minValue; }
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
			moveCursor(-_cursorPos);
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
		sf::RenderWindow& _window;
		Vector<Button> _buttons;
		Vector<TextBox> _textBoxes;
		InputBox _nameBox;
		TickBox _tickBox;
		IntegerInputBox _intBox;
		sf::Mouse _mouse;

		void _handleStartMenuEvents()
		{
			sf::Event event;
			while (_window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed)
					_window.close();
			}
		}

		void _displayStartMenu()
		{
			_window.clear();
			_textBoxes.drawAll(_window);
			_buttons.drawAll(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));
			_nameBox.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));
			_tickBox.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));
			_intBox.draw(_window, static_cast<sf::Vector2f>(_mouse.getPosition(_window)));
			_window.display();
		}

	public:
		StartMenuUI(sf::RenderWindow& window) :
			_window(window),
			_nameBox(sf::Vector2f(400.0, 50.0), sf::Vector2f(300.0, 50.0), 16, "NewAtlas", Defined::DefaultFont, Defined::AlphaNumeric, sf::Color(20, 20, 20, 255)),
			_tickBox(sf::Vector2f(20.0, 20.0), sf::Vector2f(300.0, 130.0), false, sf::Color(20, 20, 20, 255)),
			_intBox(sf::Vector2f(60.0, 40.0), sf::Vector2f(300.0, 170.0), 16, -100, 100, Defined::DefaultFont, sf::Color(20, 20, 20, 255), sf::Color(255, 255, 0, 255))
		{
			_textBoxes.add(new TextBox(sf::Vector2f(50.0, 200.0), 16, "Recent files:", Defined::DefaultFont, sf::Color(255, 255, 0, 255)));
			_buttons.add(new TextButton(sf::Vector2f(200.0, 50.0), sf::Vector2f(50.0, 50.0), 16, "Create New Atlas", Defined::DefaultFont, TextAlignment::Left, sf::Color(20, 20, 20, 255)));
			_buttons.add(new TextButton(sf::Vector2f(200.0, 50.0), sf::Vector2f(50.0, 120.0), 16, "Open Existing Atlas", Defined::DefaultFont, TextAlignment::Left, sf::Color(20, 20, 20, 255)));
		}

		void run()
		{
			while (_window.isOpen())
			{
				_handleStartMenuEvents();
				_displayStartMenu();
			}
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
			_startMenu.run();
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
