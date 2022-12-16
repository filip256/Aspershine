#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <shlwapi.h>
#include <thread>
#include <atomic>
#include <locale>
#include <codecvt>
#include <list>
#include <shlobj_core.h>

#define FILE_MAX_PATH 4096
#define VERSION "0.0.1"

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

	inline std::wstring toWide(const std::string& str)
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;

		return converterX.from_bytes(str);
	}

	inline std::string toString(const std::wstring& wstr)
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;

		return converterX.to_bytes(wstr);
	}

	std::vector<std::string> separatePaths(const std::string& str)
	{
		bool multiple = false;
		std::vector<std::string> result;
		size_t i = str.find('\n'), last = i;
		std::string aux = str.substr(0, i);
		++i;
		while (i < str.size())
		{
			if (str[i] == '\n')
			{
				result.push_back(aux + '\\' + str.substr(last + 1, i - last - 1));
				last = i;
				multiple = true;
			}
			++i;
		}
		if (multiple)
			result.push_back(aux + '\\' + str.substr(last + 1));
		else
			result.push_back(aux);
		return result;
	}

	std::wstring replaceWithEndl(const wchar_t* str)
	{
		if (str[0] == '\0')
			return std::wstring();

		std::wstring result;
		size_t i = 0;
		while (true)
		{
			if (str[i] == '\0')
			{
				if (str[i + 1] == '\0')
					return result;
				result += '\n';
			}
			else
				result += str[i];
			++i;
		}
		return result;
	}

	template<class T, size_t N> 
	class UndoStack
	{
		// maybe use vector and allow the limit to be surpassed by a value before delete happens
		std::vector<T> _content;
		size_t _idx = 0;

	public:
		UndoStack()
		{
			_content.reserve(N + MAX_OVER);
		}

		inline void push(const T& item) 
		{ 
			if (_content.size() == N + MAX_OVER)
				_content.erase(_content.begin(), _content.begin() + MAX_OVER);
			_content.push_back(item); 
		}
		inline void pop()
		{ 
			if (_idx == _content.size() - 1)
				--_idx;
			_content.pop_back(); 	
		}
		inline T& top() { return _content.back(); }
		inline T& getCurrent() { return _content[_idx--]; }

		static const size_t MAX_OVER = 10;
	};
}



namespace os
{
	class OpenFileDialog
	{
		std::atomic<bool> _ready;
		wchar_t _path[FILE_MAX_PATH] = { 0 };
		std::thread *_thr = nullptr;
		OPENFILENAME _ofn = { sizeof(OPENFILENAME) };

		static void _kern(OPENFILENAME *ofn, std::atomic<bool>* ready)
		{
			GetOpenFileName(ofn);
			*ready = true;
		}

	public:
		OpenFileDialog(const HWND& hwnd, const wchar_t *filter = L"All files\0*.*\0", const wchar_t *defaultExtension = nullptr, const bool allowMultiple = false)
		{
			_ofn.hwndOwner = hwnd;
			_ofn.lpstrTitle = L"Open Atlas";
			_ofn.lpstrFilter = filter;
			_ofn.lpstrDefExt = L"txt";
			_ofn.lpstrFile = _path;
			_ofn.nMaxFile = FILE_MAX_PATH;
			if (allowMultiple) _ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;
			else _ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
		}

		~OpenFileDialog()
		{
			if (_thr)
				delete _thr;
		}

		void start()
		{
			if (!_thr)
			{
				memset(_path, 0, FILE_MAX_PATH);
				_ready = false;
				_thr = new std::thread(_kern, &_ofn, &_ready);
			}
		}

		bool isReady() const { return _ready.load(); }
		bool isStarted() const { return _thr != nullptr; }

		std::wstring getPath()
		{
			if (_thr)
			{
				_thr->join();
				delete _thr;
				_thr = nullptr;
			}

			return util::replaceWithEndl(_path);
		}
	};
	class SaveFileDialog
	{
		std::atomic<bool> _ready;
		wchar_t _path[FILE_MAX_PATH] = { 0 };
		std::thread *_thr = nullptr;
		OPENFILENAME _ofn = { sizeof(OPENFILENAME) };

		static void _kern(OPENFILENAME *ofn, std::atomic<bool>* ready)
		{
			GetSaveFileName(ofn);
			*ready = true;
		}

	public:
		SaveFileDialog(const HWND& hwnd, const wchar_t *filter = L"All files\0*.*\0", const wchar_t *defaultExtension = nullptr, const bool allowMultiple = false)
		{
			_ofn.hwndOwner = hwnd;
			_ofn.lpstrTitle = L"Open Atlas";
			_ofn.lpstrFilter = filter;
			_ofn.lpstrDefExt = L"txt";
			_ofn.lpstrFile = _path;
			_ofn.nMaxFile = FILE_MAX_PATH;
			if (allowMultiple) _ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;
			else _ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
		}

		~SaveFileDialog()
		{
			if (_thr)
				delete _thr;
		}

		void start()
		{
			if (!_thr)
			{
				memset(_path, 0, FILE_MAX_PATH);
				_ready = false;
				_thr = new std::thread(_kern, &_ofn, &_ready);
			}
		}

		bool isReady() const { return _ready.load(); }
		bool isStarted() const { return _thr != nullptr; }

		std::wstring getPath()
		{
			if (_thr)
			{
				_thr->join();
				delete _thr;
				_thr = nullptr;
			}

			return util::replaceWithEndl(_path);
		}
	};
	class SelectFolderDialog
	{
		std::atomic<bool> _ready;
		wchar_t _path[FILE_MAX_PATH] = { 0 };
		std::thread *_thr = nullptr;
		BROWSEINFO _bi;

		static void _kern(BROWSEINFO *bi, std::atomic<bool>* ready, wchar_t* path)
		{
			SHGetPathFromIDList(SHBrowseForFolder(bi), path);
			*ready = true;
		}

	public:
		SelectFolderDialog(const HWND& hwnd)
		{
			_bi.hwndOwner = hwnd;
			_bi.lpszTitle = L"Save Atlas";
			_bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		}

		~SelectFolderDialog()
		{
			if (_thr)
				delete _thr;
		}

		void start()
		{
			if (!_thr)
			{
				memset(_path, 0, FILE_MAX_PATH);
				_ready = false;
				_thr = new std::thread(_kern, &_bi, &_ready, _path);
			}
		}

		bool isReady() const { return _ready.load(); }
		bool isStarted() const { return _thr != nullptr; }

		std::wstring getPath()
		{
			if (_thr)
			{
				_thr->join();
				delete _thr;
				_thr = nullptr;
			}

			return _path;
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

	bool directoryExists(LPCTSTR szPath)
	{
		DWORD dwAttrib = GetFileAttributes(szPath);

		return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}

	std::wstring getAvailableDir(std::wstring path)
	{
		unsigned short int cnt = 1;
		std::wstring copyTemp = path;
		while (directoryExists(path.c_str()))
		{
			path = copyTemp + L'(' + std::to_wstring(cnt) + L')';
			++cnt;
		}
		return path;
	}

	std::string getWD()
	{
		wchar_t result[MAX_PATH];
		GetModuleFileName(NULL, result, MAX_PATH);
		return util::toString(std::wstring(result).substr(0, std::wstring(result).rfind('\\')));
	}

}

namespace img
{
	enum RevertAction
	{

	};
	typename std::pair<const RevertAction, void*> Revertable;

	class UndoHelper
	{

	};

	sf::Vector2f operator* (const sf::Vector2f first, const sf::Vector2f second) { return sf::Vector2f(first.x * second.x, first.y * second.y); }

	class ImageBoxData
	{
	public:
		std::string nameTag;
		sf::Vector2f position, scale;
		bool isValid = false;

		ImageBoxData()
		{}

		ImageBoxData(const std::string& nameTag, const sf::Vector2f& position, const sf::Vector2f& scale) :
			nameTag(nameTag),
			position(position),
			scale(scale)
		{}
	};

	class ImageBox
	{
		bool _isSelected = false;
		mutable bool _isOverlapped = false;
		std::string _nameTag;
		sf::Texture _tx;
		sf::Sprite _sp;

	public:
		ImageBox(const std::string& nameTag, const std::string& sourcefile = "") :
			_nameTag(nameTag)
		{
			setImage(sourcefile);
		}

		ImageBox(const ImageBox& other) :
			_tx(other._tx),
			_sp(_tx)
		{}

		ImageBox(const ImageBoxData& data, const std::string& imageDir) :
			_nameTag(data.nameTag)
		{
			setImage(imageDir + "\\" + _nameTag + ".png");
			_sp.setPosition(data.position);
			_sp.setScale(data.scale);
		}

		void setImage(const std::string& sourcefile)
		{
			if (sourcefile.length() == 0 || !_tx.loadFromFile(sourcefile))
				_tx.loadFromFile("iconsc.png");
			_sp.setTexture(_tx);
		}

		inline sf::Vector2f getPosition() const { return _sp.getPosition(); }
		inline sf::FloatRect getBounds() const { return _sp.getGlobalBounds(); }
		inline std::string getNameTag() const { return _nameTag; }
		inline bool isSelected() const { return _isSelected; }
		inline bool isOverlapped() const { return _isOverlapped; }

		inline void setSelect(const bool boolean) { _isSelected = boolean; }
		inline void setOverlap(const bool boolean) const { _isOverlapped = boolean; }
		inline void setPosition(const sf::Vector2f& position) { _sp.setPosition(position);}
		inline void move(const sf::Vector2f& delta) { _sp.move(delta); }
		inline void setScale(const sf::Vector2f& scale) { _sp.setScale(scale); }
		inline void applyScale(const sf::Vector2f& scale) { _sp.setScale(_sp.getScale() * scale); }

		inline bool contains(const sf::Vector2f& coord) const { return _sp.getGlobalBounds().contains(coord); }
		inline bool intersects(const sf::FloatRect& rect) const { return rect.intersects(_sp.getGlobalBounds()); }

		inline const sf::Sprite& getDrawObject() const 
		{
			return _sp;
		}
		inline const sf::Image getImage() const
		{
			if (_sp.getScale() == sf::Vector2f(1.0, 1.0))
				return _tx.copyToImage();

			const sf::Image original = _tx.copyToImage();
			sf::Image result;
			result.create(_sp.getGlobalBounds().width, _sp.getGlobalBounds().height, sf::Color(0, 0, 0, 0));
			const sf::Vector2u originalImageSize(original.getSize());
			const sf::Vector2u resizedImageSize(_sp.getGlobalBounds().width, _sp.getGlobalBounds().height);
			for (unsigned int y = 0; y < resizedImageSize.y; ++y)
			{
				for (unsigned int x = 0; x < resizedImageSize.x; ++x)
				{
					unsigned int origX{ static_cast<unsigned int>(static_cast<double>(x) / resizedImageSize.x * originalImageSize.x) };
					unsigned int origY{ static_cast<unsigned int>(static_cast<double>(y) / resizedImageSize.y * originalImageSize.y) };
					result.setPixel(x, y, original.getPixel(origX, origY));
				}
			}
			return result;
		}

		void draw(sf::RenderWindow& window)
		{
			window.draw(_sp);
			if (_isOverlapped)
			{
				sf::FloatRect bounds = _sp.getGlobalBounds();
				sf::RectangleShape outline(sf::Vector2f(bounds.width, bounds.height));
				outline.setPosition(bounds.left, bounds.top);
				outline.setFillColor(sf::Color(255, 0, 0, 75));
				outline.setOutlineColor(sf::Color(255, 0, 0, 255));
				outline.setOutlineThickness(-1);
				window.draw(outline);
			}
			if (_isSelected)
			{
				sf::FloatRect bounds = _sp.getGlobalBounds();
				sf::RectangleShape outline(sf::Vector2f(bounds.width, bounds.height));
				outline.setPosition(bounds.left, bounds.top);
				outline.setFillColor(sf::Color(255, 255, 255, 75));
				outline.setOutlineColor(sf::Color(0, 0, 0, 255));
				outline.setOutlineThickness(1);
				window.draw(outline);
			}
		}

		std::string serialize(const sf::Vector2f& scaleOffset = sf::Vector2f(1.0, 1.0)) const
		{
			return _nameTag + ':'
				+ std::to_string(_sp.getPosition().x) + ':'
				+ std::to_string(_sp.getPosition().y) + ':'
				+ std::to_string(_sp.getScale().x / scaleOffset.x) + ':'
				+ std::to_string(_sp.getScale().y / scaleOffset.x);
		}
		void saveToFile(const std::string& path) const
		{
			_tx.copyToImage().saveToFile(path + "\\" + _nameTag + ".png");
		}

		static ImageBoxData deserialize(const std::string& line)
		{
			ImageBoxData data;

			try
			{
				size_t last, next = line.find(':');
				data.nameTag = line.substr(0, next);
				last = next;

				next = line.find(':', last + 1);
				data.position.x = std::stof(line.substr(last + 1, next));
				last = next;

				next = line.find(':', last + 1);
				data.position.y = std::stof(line.substr(last + 1, next));
				last = next;

				next = line.find(':', last + 1);
				data.scale.x = std::stof(line.substr(last + 1, next));
				last = next;

				data.scale.y = std::stof(line.substr(last + 1));
			}
			catch (...)
			{
				return data;
			}

			data.isValid = true;
			return data;
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
		bool _anySelected = false;
		std::vector<ImageBox*> _images;
		sf::Vector2f _position;
		sf::Vector2f _scale;

		sf::IntRect getBounds() const
		{
			sf::IntRect maxRect(0, 0, 0, 0);
			for (size_t i = 0; i < _images.size(); ++i)
			{
				const int w = _images[i]->getPosition().x + _images[i]->getBounds().width;
				const int h = _images[i]->getPosition().y + _images[i]->getBounds().height;
				if (_images[i]->getPosition().x < maxRect.left)
					maxRect.left = _images[i]->getPosition().x;
				if (_images[i]->getPosition().y < maxRect.top)
					maxRect.top = _images[i]->getPosition().y;
				if (w > maxRect.width)
					maxRect.width = w;
				if (h > maxRect.height)
					maxRect.height = h;
			}
			return maxRect;
		}

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
				addImage(*other._images[i], false);
			other._images.clear();
		}

		inline void addImage(const std::string& name, const std::string& sourcefile, const bool resetProperties = true)
		{
			_images.push_back(new ImageBox(name, sourcefile));

			if (resetProperties)
			{
				_images.back()->setScale(_scale);
				if (_images.size() > 1)
					_images.back()->setPosition(sf::Vector2f(_images[_images.size() - 2]->getPosition() + sf::Vector2f(20.0, 20.0)));
				else
					_images.back()->setPosition(_position);
			}
		}
		inline void addImage(const std::string& sourcefile)
		{
			const std::string aux = sourcefile.substr(sourcefile.rfind('\\') + 1);
			addImage(aux.substr(0, aux.rfind('.')), sourcefile);
		}
		inline void addImage(ImageBox& image, const bool resetProperties = true)
		{
			_images.push_back(&image);

			if (resetProperties)
			{
				_images.back()->setScale(_scale);
				if (_images.size() > 1)
					_images.back()->setPosition(_images[_images.size() - 2]->getPosition() + sf::Vector2f(20.0, 20.0));
				else
					_images.back()->setPosition(_position);
			}
		}

		inline void deleteImage(const size_t idx)
		{
			delete _images[idx];
			_images.erase(_images.begin() + idx);
		}
		void deleteImages(const bool selectedOnly = false)
		{
			for (size_t i = 0; i < _images.size(); ++i)
				if (!selectedOnly || _images[i]->isSelected())
				{
					deleteImage(i);
					--i;
				}
		}

		inline sf::Vector2f getScale() const { return _scale; }
		inline size_t size() const { return _images.size(); }
		inline bool anySelected() const { return _anySelected; }
		bool anyContains(const sf::Vector2f& point)
		{
			for (size_t i = 0; i < _images.size(); ++i)
				if (_images[i]->contains(point))
					return true;
			return false;
		}

		inline void setScale(const sf::Vector2f& scale, const bool selectedOnly = false)
		{
			if(!selectedOnly) _scale = scale;
			for (size_t i = 0; i < _images.size(); ++i)
				if (!selectedOnly || _images[i]->isSelected())
					_images[i]->setScale(scale);
		}
		inline void setPosition(const sf::Vector2f& position)
		{
			const sf::Vector2f delta = position - _position;
			_position = position;
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->move(delta);
		}
		inline void moveImages(const sf::Vector2f& offset, const bool selectedOnly = false)
		{
			for (size_t i = 0; i < _images.size(); ++i)
				if (!selectedOnly || _images[i]->isSelected())
					_images[i]->move(offset);
		}
		inline void applyScale(const sf::Vector2f& scale, const bool selectedOnly = false)
		{
			if(!selectedOnly) _scale = _scale * scale;
			for (size_t i = 0; i < _images.size(); ++i)
				if(!selectedOnly || _images[i]->isSelected())
					_images[i]->applyScale(scale);
		}

		void clearOverlapped() const
		{
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->setOverlap(false);
		}
		void findOverlapped() const
		{
			if (_images.size())
			{
				clearOverlapped();
				for (size_t i = 0; i < _images.size() - 1; ++i)
					for (size_t j = i + 1; j < _images.size(); ++j)
						if (_images[i]->getBounds().intersects(_images[j]->getBounds()))
						{
							_images[i]->setOverlap(true);
							_images[j]->setOverlap(true);
						}
			}
		}
		void selectAll()
		{
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->setSelect(true);
			_anySelected = true;
		}
		void deselectAll()
		{
			if (_anySelected)
			{
				for (size_t i = 0; i < _images.size(); ++i)
					_images[i]->setSelect(false);
				_anySelected = false;
			}
		}
		bool select(const sf::Vector2f& coord, const bool multiSelect = false)
		{
			if(!multiSelect)
				deselectAll();
			for(int i = _images.size() - 1; i >= 0; --i)
				if (_images[i]->contains(coord))
				{
					_images[i]->setSelect(true);
					_anySelected = true;
					return true;
				}
			return false;
		}
		void select(const sf::FloatRect& rect)
		{
			for (size_t i = 0; i < _images.size(); ++i)
				if (_images[i]->intersects(rect))
				{
					_images[i]->setSelect(true);
					_anySelected = true;
				}
				else
					_images[i]->setSelect(false);
		}
		int findImage(const sf::Vector2f& point)
		{
			for (int i = _images.size() - 1; i >= 0; --i)
				if (_images[i]->contains(point))
					return i;
			return -1;
		}

		inline void draw(sf::RenderWindow& window, const bool showOverlapped = false) const 
		{
			if(showOverlapped) findOverlapped();
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->draw(window);
		}

		const ImageBox& operator[](std::size_t idx) const { return *_images[idx]; }

		bool exportToImage(const std::string& path) const
		{
			if (_images.size() == 0)
				return false;

			const sf::IntRect bounds = getBounds();
			if (bounds == sf::IntRect(0, 0, 0, 0))
				return false;

			const sf::Vector2f offset(0.0 - bounds.left, 0.0 - bounds.top);
			sf::Image result;
			result.create(bounds.width + offset.x, bounds.height + offset.y, sf::Color(0, 0, 0, 0));
			for (size_t i = 0; i < _images.size(); ++i)
			{
				sf::Image image(std::move(_images[i]->getImage()));
				for (unsigned int y = 0; y < image.getSize().y; ++y)
					for (unsigned int x = 0; x < image.getSize().x; ++x)
					{
						result.setPixel(_images[i]->getPosition().x + offset.x + x, _images[i]->getPosition().y + offset.y + y, image.getPixel(x, y));
					}
			}
			return result.saveToFile(path);
		}
		void saveToFile(const std::string& path) const
		{
			CreateDirectoryA(path.c_str(), NULL);
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->saveToFile(path);
		}
		void loadFromFile(std::ifstream& fileIn, const std::string& imageDir)
		{
			std::string line;
			while (getline(fileIn, line))
			{
				ImageBoxData data = ImageBox::deserialize(line);
				if (data.isValid)
				{
					_images.push_back(new ImageBox(data, imageDir));
					std::cout<<_images.back()->getDrawObject().getScale().x << '\n';
				}
			}
		}
	};
	std::ostream& operator<<(std::ostream& os, const ImageVector& obj)
	{
		for (size_t i = 0; i < obj.size(); ++i)
			os << obj[i].serialize(obj.getScale()) << '\n';
		return os;
	}
}

namespace ui
{
	using namespace img;

	class FileImage
	{
	public:
		std::string name, path;

		FileImage(const std::string& name, const std::string& path) :
			name(name),
			path(path)
		{}
	};
	class RecentFileStore
	{
		std::vector<FileImage> _files;

	public:
		RecentFileStore()
		{
			_files.reserve(10);
		}

		void load(const std::string& path)
		{
			_files.clear();
			std::ifstream in(path);
			if (!in)
				return;

			std::string line;
			unsigned int cnt = 0;
			while (cnt < 10 && getline(in, line))
			{
				size_t sep = line.find(':');
				_files.emplace_back(std::move(FileImage(line.substr(0, sep), line.substr(sep + 1))));
				++cnt;
			}
			in.close();
		}
		void dump(const std::string& path)
		{
			std::cout << path << '\n';
			std::ofstream out(path);
			if (!out)
				return;

			for (size_t i = 0; i < _files.size(); ++i)
				out << _files[i].name << ':' << _files[i].path << '\n';

			out.close();
		}

		void add(const std::string& name, const std::string& path)
		{
			for (size_t i = 0; i < _files.size(); ++i)
			{
				if (_files[i].path == path)
				{
					if (i != 0)
					{
						_files.insert(_files.begin(), _files[i]);
						_files.erase(_files.begin() + i + 1);
					}
					return;
				}
			}

			if(_files.size() >= 10)
				_files.pop_back();
			_files.emplace(_files.begin(), FileImage(name, path));
		}

		inline size_t size() const { return _files.size(); }

		const FileImage& operator[](std::size_t idx) const { return _files[idx]; }
	};

	class Defined
	{
	public:
		static RecentFileStore RecentFiles;
		static const unsigned int CursorBlinkInterval = 500;

		static const std::string AlphaNumeric;
		static const std::string NonNegativeNumeric;
		static const std::string Numeric;

		static sf::Font DefaultFont;

		static const sf::Color DarkGrey;
		static const sf::Color RedDarkGrey;
		static const sf::Color Grey;
		static const sf::Color DarkCyan;


		static bool load()
		{
			if (!DefaultFont.loadFromFile("texgyreadventor-regular.otf"))
				return false;

			RecentFiles.load(os::getWD() + "\\recents.txt");

			return true;
		}
		static void release()
		{
			RecentFiles.dump(os::getWD() + "\\recents.txt");
		}
	};
	const std::string Defined::AlphaNumeric = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	const std::string Defined::NonNegativeNumeric = "0123456789";
	const std::string Defined::Numeric = "-0123456789";
	sf::Font Defined::DefaultFont;
	const sf::Color Defined::DarkGrey = sf::Color(20, 20, 20, 255);
	const sf::Color Defined::RedDarkGrey = sf::Color(80, 20, 20, 255);
	const sf::Color Defined::Grey = sf::Color(50, 50, 50, 255);
	const sf::Color Defined::DarkCyan = sf::Color(11, 129, 133, 255);
	RecentFileStore Defined::RecentFiles;

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

		ADD_IMAGE,
		DELETE_IMAGE,
		DELETE_ALL,
		SELECT_ALL,

		CONTINUE_1,
		CONTINUE_2
	};
	class ActionResult
	{
	public:
		const ActionEvent eventTag;
		const std::string obj;

		ActionResult(const ActionEvent event = ActionEvent::NONE, const std::string& object = "") :
			eventTag(event),
			obj(object)
		{}

		inline ActionEvent getActionEvent() const { return eventTag; }
	};
	inline bool operator==(const ActionResult& lhs, const ActionResult& rhs) { return lhs.eventTag == rhs.eventTag; }

	class Background
	{
	protected:
		sf::RectangleShape _back;

	public:
		Background(const sf::Vector2f& size, const sf::Vector2f& position, const sf::Color& color) :
			_back(size)
		{
			_back.setPosition(position);
			_back.setFillColor(color);
		}

		inline bool contains(const sf::Vector2f& point) const { return _back.getGlobalBounds().contains(point); }

		inline void setPosition(const sf::Vector2f& position) { _back.setPosition(position); }
		inline void setFillColor(const sf::Color& color) { _back.setFillColor(color); }

		inline void draw(sf::RenderWindow& window) const
		{
			window.draw(_back);
		}
	};
	class SelectedArea : public Background
	{
		bool _isInUse = false;
	public:
		SelectedArea() :
			Background(sf::Vector2f(0.0, 0.0), sf::Vector2f(0.0, 0.0), sf::Color(11, 129, 133, 30))
		{
			_back.setOutlineThickness(1);
			_back.setOutlineColor(Defined::DarkCyan);
		}

		inline void setInUse(const bool boolean) { _isInUse = boolean; }
		inline void clear() { _back.setSize(sf::Vector2f(0.0, 0.0)); _isInUse = false; }
		inline void changeEndpoint(const sf::Vector2f& point) { _back.setSize(point - _back.getPosition()); }
		inline void setSize(const sf::Vector2f& size) { _back.setSize(size); }
		inline void resizeBy(const sf::Vector2f& size) { _back.setSize(_back.getSize() + size); }

		inline sf::FloatRect getBounds() const { return _back.getGlobalBounds(); }
		inline bool isInUse() const { return _isInUse; }

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

	class DragNDropHelper
	{
		bool _isInUse = false;
		sf::Vector2f _initialPos;

	public:

		inline void use(const sf::Vector2f& point) { _isInUse = true; _initialPos = point; }
		inline void clear() { _isInUse = false; }
		inline void setOrigin(const sf::Vector2f& point) { _initialPos = point; }

		inline bool isInUse() const { return _isInUse; }
		inline sf::Vector2f getDelta(const sf::Vector2f& point) const { return point - _initialPos; }
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

		inline sf::Vector2f getPosition() const { return _back.getPosition(); }
		inline sf::Vector2f getSize() const { return _back.getSize(); }

		inline virtual void setPosition(const sf::Vector2f& position)
		{
			_back.setPosition(position);
		}

		inline bool contains(const sf::Vector2f& point) const { return _back.getGlobalBounds().contains(point); }

		inline const ActionEvent click() const { return _returnEvent; }

		virtual void draw(sf::RenderWindow& window, const sf::Vector2f& point)
		{
			_checkHovering(point);
			window.draw(_back);
		}

		static inline sf::Vector2f below(const Button& object)
		{
			return sf::Vector2f(object.getPosition().x, object.getPosition().y + object.getSize().y + 1.0);
		}
		static inline sf::Vector2f after(const Button& object)
		{
			return sf::Vector2f(object.getPosition().x + object.getSize().x + 1.0, object.getPosition().y);
		}
	};
	class TickBox : public Button
	{
		bool _isTicked = false;
		sf::Sprite _tickSprite;

	public:
		TickBox(const sf::Vector2f& size, const sf::Vector2f& position, const ActionEvent returnEvent, const bool isTicked = false, const sf::Color& fillColor = sf::Color(0, 0, 0, 0)) :
			Button(size, position, returnEvent, fillColor),
			//_tickSprite(Defined::Icons),
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
		const TextAlignment _alignment;

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
			_text(text, font, textSize),
			_alignment(alignment)
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

		inline void setPosition(const sf::Vector2f& position) override
		{
			if (_alignment == TextAlignment::Center)
				_text.setPosition(static_cast<int>(position.x + (_back.getSize().x - _text.getGlobalBounds().width) / 2.0), static_cast<int>(position.y + (_back.getSize().y - _text.getGlobalBounds().height) / 2.0));
			else if (_alignment == TextAlignment::Left)
				_text.setPosition(position.x + 5, static_cast<int>(position.y + (_back.getSize().y - _text.getGlobalBounds().height) / 2.0));
			else if (_alignment == TextAlignment::Right)
				_text.setPosition(position.x + _back.getSize().x - _text.getGlobalBounds().width - 5, static_cast<int>(position.y + (_back.getSize().y - _text.getGlobalBounds().height) / 2.0));
			_back.setPosition(position);
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

		inline void clear()
		{
			for (size_t i = 0; i < _content.size(); ++i)
				delete _content[i];
			_content.clear();
		}

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

		inline const int contains(const sf::Vector2f& point)
		{
			for (int i = _content.size() - 1; i >= 0; --i)
				if (_content[i]->contains(point))
					return i;
			return -1;
		}

		inline void clear()
		{
			for (size_t i = 0; i < _content.size(); ++i)
				delete _content[i];
			_content.clear();
		}

		void drawAll(sf::RenderWindow& window, const sf::Vector2f& point)
		{
			for (size_t i = 0; i < _content.size(); ++i)
				_content[i]->draw(window, point);
		}
	};

	class OptionsMenu
	{
		bool _isOpen = false;

	protected:
		Background _back;

	public:
		OptionsMenu() :
			_back(sf::Vector2f(100.0, 20.0), sf::Vector2f(0.0, 0.0), Defined::Grey)
		{}

		inline void open(const sf::Vector2f& position)
		{
			_isOpen = true;
			setPosition(position);
		}
		inline void close() { _isOpen = false; }
		inline bool isOpen() const { return _isOpen; }

		virtual void setPosition(const sf::Vector2f& position) = 0;
	};
	class AtlasOptionsMenu : public OptionsMenu
	{
		TextButton _addImageB, _selectAllB, _deleteAllB;


	public:
		AtlasOptionsMenu() :
			OptionsMenu(),
			_addImageB(sf::Vector2f(100.0, 24.0), sf::Vector2f(0.0, 0.0), 14, "Add Image", ActionEvent::NONE, Defined::DefaultFont, TextAlignment::Center, Defined::Grey),
			_selectAllB(sf::Vector2f(100.0, 24.0), Button::below(_addImageB), 14, "Select All", ActionEvent::NONE, Defined::DefaultFont, TextAlignment::Center, Defined::Grey),
			_deleteAllB(sf::Vector2f(100.0, 24.0), Button::below(_selectAllB), 14, "Delete All", ActionEvent::NONE, Defined::DefaultFont, TextAlignment::Center, Defined::RedDarkGrey)
		{}

		void setPosition(const sf::Vector2f& position) override
		{
			_back.setPosition(position);
			_addImageB.setPosition(position);
			_selectAllB.setPosition(Button::below(_addImageB));
			_deleteAllB.setPosition(Button::below(_selectAllB));
		}

		ActionResult handleEvent(const sf::Event& event, const sf::Vector2f& mousePosition)
		{
			if (event.type == sf::Event::MouseButtonPressed)
			{
				if (_addImageB.contains(mousePosition))
					return ActionEvent::ADD_IMAGE;
				else if (_selectAllB.contains(mousePosition))
					return ActionEvent::SELECT_ALL;
				else if (_deleteAllB.contains(mousePosition))
					return ActionEvent::DELETE_ALL;
			}
			return ActionEvent::NONE;
		}

		void draw(sf::RenderWindow& window, const sf::Vector2f& point)
		{
			_back.draw(window);
			_addImageB.draw(window, point);
			_selectAllB.draw(window, point);
			_deleteAllB.draw(window, point);
		}
	};
	class ImageOptionsMenu : public OptionsMenu
	{
		TextButton _deleteB;


	public:
		ImageOptionsMenu() :
			OptionsMenu(),
			_deleteB(sf::Vector2f(100.0, 24.0), sf::Vector2f(0.0, 0.0), 14, "Delete", ActionEvent::NONE, Defined::DefaultFont, TextAlignment::Center, Defined::RedDarkGrey)
		{}

		void setPosition(const sf::Vector2f& position) override
		{
			_back.setPosition(position);
			_deleteB.setPosition(position);
		}

		ActionResult handleEvent(const sf::Event& event, const sf::Vector2f& mousePosition)
		{
			if (event.type == sf::Event::MouseButtonPressed)
			{
				if (_deleteB.contains(mousePosition))
					return ActionEvent::DELETE_IMAGE;
			}
			return ActionEvent::NONE;
		}

		void draw(sf::RenderWindow& window, const sf::Vector2f& point)
		{
			_back.draw(window);
			_deleteB.draw(window, point);
		}
	};

	class Atlas
	{
		std::string _name;
		ImageVector _images;
		Background _back;

	public:
		Atlas(const std::string& name = "New Atlas") :
			_name(name),
			_back(sf::Vector2f(1180.0, 700.0), sf::Vector2f(10.0, 74.0), sf::Color(100, 100, 100, 255))
		{
			_images.setPosition(sf::Vector2f(10.0, 74.0));
		}

		inline ImageVector& accessData() { return _images; }

		inline std::string getName() const { return _name; }
		inline void setName(const std::string& name) { _name = name; }

		inline bool contains(const sf::Vector2f& point) const { return _back.contains(point); }

		inline void draw(sf::RenderWindow& window, const bool showOverlapped = false) const
		{
			_back.draw(window);
			_images.draw(window, showOverlapped);
		}

		void saveToFile(const std::string& path, const bool allowOverwrite) const
		{
			std::string temp;
			if (allowOverwrite)
				temp = path + "\\" + _name;
			else
				temp = util::toString(os::getAvailableDir(util::toWide(path + "\\" + _name)));

			CreateDirectoryA(temp.c_str(), NULL);
			std::ofstream out(temp + "\\" + _name + ".atl");
			if (!out)
				return;

			out << VERSION<< '\n' << _name << '\n' << _images;
			out.close();

			_images.saveToFile(temp + "\\" + _name + "_img");
		}
		bool exportToImage(const std::string& path) const
		{
			return _images.exportToImage(path);
		}

		static Atlas loadFromFile(const std::string& filepath)
		{
			std::ifstream in(filepath);
			Atlas atl;

			if (!in)
				return atl;

			std::string version;
			getline(in, version);

			std::string line;
			getline(in,line);
			atl.setName(line);

			atl._images.loadFromFile(in, filepath.substr(0, filepath.rfind('\\') + 1) + atl._name + "_img");

			in.close();

			Defined::RecentFiles.add(atl.getName(), filepath);
			return atl;
		}
	};

	class EditorUI
	{
		sf::RenderWindow& _window;
		sf::Mouse _mouse;
		std::vector<Atlas> _tabs;
		size_t _frontTab = 0;

		DragNDropHelper _dragger;
		Background _generalBg, _upBg, _downBg, _leftBg, _rightBg;
		SelectableTextButton _fileB, _viewB, _settingsB;
		Vector<Button> _tabButtons;
		AtlasOptionsMenu _optionsMenu;
		ImageOptionsMenu _imageMenu;
		SelectedArea _selection;

		os::OpenFileDialog _openFileDialog;
		os::SaveFileDialog _saveFileDialog;
		os::SelectFolderDialog _selectFolderDialog;

		inline void _setZoomPrecision(sf::Vector2f& up, sf::Vector2f& down)
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
			{
				if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
				{
					up = sf::Vector2f(1.0005, 1.0005);
					down = sf::Vector2f(0.9995, 0.9995);
				}
				else
				{
					up = sf::Vector2f(1.001, 1.001);
					down = sf::Vector2f(0.999, 0.999);
				}
			}
			else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
			{
				up = sf::Vector2f(1.01, 1.01);
				down = sf::Vector2f(0.99, 0.99);
			}
			else
			{
				up = sf::Vector2f(1.1, 1.1);
				down = sf::Vector2f(0.9, 0.9);
			}
		}

		ActionEvent _handleDefaultEvents()
		{
			sf::Event event;
			bool rightClick = false;
			while (_window.pollEvent(event))
			{
				const sf::Vector2f mousePos = static_cast<sf::Vector2f>(_mouse.getPosition(_window));
				ActionEvent optionsResult = ActionEvent::NONE;
				if (_optionsMenu.isOpen())
				{
					optionsResult = _optionsMenu.handleEvent(event, mousePos).getActionEvent();
					if (optionsResult == ActionEvent::ADD_IMAGE)
						_openFileDialog.start();
					else if (optionsResult == ActionEvent::SELECT_ALL)
						_tabs[_frontTab].accessData().selectAll();
					else if (optionsResult == ActionEvent::DELETE_ALL)
						_tabs[_frontTab].accessData().deleteImages(true);
				}
				else if (_imageMenu.isOpen())
				{
					optionsResult = _imageMenu.handleEvent(event, mousePos).getActionEvent();
					if (optionsResult == ActionEvent::DELETE_IMAGE)
					{
						_tabs[_frontTab].accessData().deleteImages(true);
					}
				}
				
				switch (event.type)
				{
				case sf::Event::MouseButtonPressed:
				{
					if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
					{
						_optionsMenu.close();
						_imageMenu.close();
						//click buttons
						const int clicked = _tabButtons.contains(mousePos);
						if (clicked != -1)
						{
						}

						if (_fileB.contains(mousePos))
						{
							_fileB.setSelected(true);
						}
						else _fileB.setSelected(false);

						if (_viewB.contains(mousePos))
						{
							_viewB.setSelected(!_viewB.isSelected());
							if (!_viewB.isSelected())
								_tabs[_frontTab].accessData().clearOverlapped();
						}
						//else _viewB.setSelected(false);

						if (_settingsB.contains(mousePos))
						{
							_settingsB.setSelected(true);
							//_selectFolderDialog.start();
							_saveFileDialog.start();
						}
						else _settingsB.setSelected(false);

						if (optionsResult != ActionEvent::SELECT_ALL && optionsResult != ActionEvent::DELETE_IMAGE)
						{
							if (_tabs[_frontTab].contains(mousePos))
							{
								if (_tabs[_frontTab].accessData().select(mousePos, sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl)))
								{
									_dragger.use(mousePos);
								}
								else
								{
									_selection.setPosition(mousePos);
									_selection.setInUse(true);
								}
							}
						}
					}
					else if(sf::Mouse::isButtonPressed(sf::Mouse::Right))
					{
						if (_tabs[_frontTab].contains(mousePos))
						{
							if (_tabs[_frontTab].accessData().anyContains(mousePos))
							{
								_imageMenu.open(mousePos + sf::Vector2f(1.0, 1.0));
								_tabs[_frontTab].accessData().select(mousePos);
								_optionsMenu.close();
							}
							else
							{
								_optionsMenu.open(mousePos + sf::Vector2f(1.0, 1.0));
								_imageMenu.close();
							}
						}
						else
						{
							_optionsMenu.close();
							_imageMenu.close();
						}
					}
				}
				break;

				case sf::Event::MouseButtonReleased:
				{
					if (_selection.isInUse())
						_selection.clear();
					else if (_dragger.isInUse())
						_dragger.clear();
				}
				break;

				case sf::Event::MouseMoved:
				{
					if (_selection.isInUse())
					{
						_selection.changeEndpoint(mousePos);
						_tabs[_frontTab].accessData().select(_selection.getBounds());
					}
					else if (_dragger.isInUse())
					{
						_tabs[_frontTab].accessData().moveImages(_dragger.getDelta(mousePos), true);
						_dragger.setOrigin(mousePos);
					}
				}
				break;

				case sf::Event::MouseWheelMoved:
				{
					sf::Vector2f up, down;
					_setZoomPrecision(up, down);
					if (event.mouseWheel.delta > 0)
						_tabs[_frontTab].accessData().applyScale(up, _tabs[_frontTab].accessData().anySelected());
					else
						_tabs[_frontTab].accessData().applyScale(down, _tabs[_frontTab].accessData().anySelected());
				}
				break;
				case sf::Event::Closed:
					return ActionEvent::WINDOW_CLOSED;
				}
			}
			return ActionEvent::NONE;
		}

		void _selectTab(size_t tabIdx)
		{
			if (tabIdx < _tabs.size())
			{
				_frontTab = tabIdx;
			}
		}

		void _displayEditor()
		{
			const sf::Vector2f mousePos = static_cast<sf::Vector2f>(_mouse.getPosition(_window));
			_window.clear();

			//_generalBg.draw(_window);

			_tabs[_frontTab].draw(_window, _viewB.isSelected());

			_upBg.draw(_window);
			_leftBg.draw(_window);
			_downBg.draw(_window);
			_rightBg.draw(_window);

			_fileB.draw(_window, mousePos);
			_viewB.draw(_window, mousePos);
			_settingsB.draw(_window, mousePos);

			_tabButtons.drawAll(_window, mousePos);

			if (_optionsMenu.isOpen())
				_optionsMenu.draw(_window, mousePos);

			if (_imageMenu.isOpen())
				_imageMenu.draw(_window, mousePos);

			_selection.draw(_window);

			_window.display();
		}

	public:
		EditorUI(sf::RenderWindow& window) :
			_window(window),
			_generalBg(sf::Vector2f(window.getSize().x, window.getSize().y), sf::Vector2f(0.0, 0.0), Defined::DarkGrey),
			_upBg(sf::Vector2f(window.getSize().x, 74.0), sf::Vector2f(0.0, 0.0), Defined::DarkGrey),
			_leftBg(sf::Vector2f(10.0, window.getSize().y), sf::Vector2f(0.0, 0.0), Defined::DarkGrey),
			_downBg(sf::Vector2f(window.getSize().x, 25.0), sf::Vector2f(0.0, 775.0), Defined::DarkGrey),
			_rightBg(sf::Vector2f(10.0, window.getSize().y), sf::Vector2f(1190.0, 0.0), Defined::DarkGrey),
			_fileB(sf::Vector2f(50.0, 24.0), sf::Vector2f(0.0, 0.0), 14, "File", ActionEvent::NONE, Defined::DefaultFont, TextAlignment::Center, Defined::DarkGrey),
			_viewB(sf::Vector2f(50.0, 24.0), Button::after(_fileB), 14, "View", ActionEvent::NONE, Defined::DefaultFont, TextAlignment::Center, Defined::DarkGrey),
			_settingsB(sf::Vector2f(90.0, 24.0), Button::after(_viewB), 14, "Settings", ActionEvent::NONE, Defined::DefaultFont, TextAlignment::Center, Defined::DarkGrey),
			_openFileDialog(window.getSystemHandle(), L"Atlas files\0.atl\0", nullptr, true),
			_saveFileDialog(window.getSystemHandle(), L"Image files (.bmp, .png, .tga, .jpg)\0.bmp;.png;.tga;.jpg\0", nullptr, false),
			_selectFolderDialog(window.getSystemHandle())
		{}

		void addAtlas(Atlas&& atlas)
		{
			_tabs.emplace_back(std::move(atlas));
			_tabButtons.add(new SelectableTextButton(sf::Vector2f(100.0, 24.0), sf::Vector2f((_tabs.size() - 1) * 100.0 + 10.0, 50.0), 14, atlas.getName(), ActionEvent::NONE, Defined::DefaultFont, TextAlignment::Left, Defined::DarkGrey));
			_selectTab(_tabs.size() - 1);
		}

		ActionResult run()
		{
			while (_window.isOpen())
			{
				const ActionResult responseEvent = _handleDefaultEvents();
				if (responseEvent == ActionEvent::WINDOW_CLOSED)
					return responseEvent;

				if (_openFileDialog.isStarted() && _openFileDialog.isReady())
				{
					std::vector<std::string> paths = util::separatePaths(util::toString(_openFileDialog.getPath()));
					for (int i = 0; i < paths.size(); ++i)
						if(paths[i].size())
							_tabs[_frontTab].accessData().addImage(paths[i]);
					_selection.clear();
					_tabs[_frontTab].accessData().deselectAll();
				}
				if (_saveFileDialog.isStarted() && _saveFileDialog.isReady())
				{
					if (_saveFileDialog.getPath().size())
						_tabs[_frontTab].exportToImage(util::toString(_saveFileDialog.getPath()));
				}
				if (_selectFolderDialog.isStarted() && _selectFolderDialog.isReady())
				{
					if (_selectFolderDialog.getPath().size())
						_tabs[_frontTab].saveToFile(util::toString(_selectFolderDialog.getPath()), true);
					Defined::RecentFiles.add(_tabs[_frontTab].getName(), util::toString(_selectFolderDialog.getPath()) + "\\" + _tabs[_frontTab].getName() + "\\" + _tabs[_frontTab].getName() + ".atl");
				}

				_displayEditor();
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

		void _loadRecentButtons()
		{
			_buttons.clear();
			for (size_t i = 0; i < Defined::RecentFiles.size(); ++i)
			{
				_buttons.add(new TextButton(
					sf::Vector2f(200.0, 30.0),
					sf::Vector2f(50.0, 220 + i * 35.0), 16,
					Defined::RecentFiles[i].name,
					ActionEvent::BUTTON_CLICKED_3,
					Defined::DefaultFont,
					TextAlignment::Left,
					sf::Color(20, 20, 20, 255),
					sf::Color(255 - i * 20, 255 - i * 20, 255 - i * 20, 255)));
			}
		}

		ActionResult _handleDefaultEvents()
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
					const int clicked = _buttons.contains(mousePos);
					if (clicked != -1)
					{
						return ActionResult(ActionEvent::CONTINUE_2, Defined::RecentFiles[clicked].path);
					}

					if (_newButton.contains(mousePos))
					{
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
			return ActionEvent::NONE;
		}
		ActionResult _handleNewAtlasEvents()
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
						_openFileDialog.start();
					}

					if (_continueButton.contains(mousePos))
					{
						return ActionResult(ActionEvent::CONTINUE_1, _nameBox.getContent());
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
			return ActionEvent::NONE;
		}

		ActionResult _handleEvents()
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
			_back1(sf::Vector2f(300.0, window.getSize().y), sf::Vector2f(0.0, 0.0), Defined::DarkGrey),
			_back2(sf::Vector2f(window.getSize().x - 300, window.getSize().y), sf::Vector2f(300.0, 0.0), sf::Color(sf::Color(40, 40, 40, 255))),
			_recentsTextBox(sf::Vector2f(50.0, 200.0), 16, "Recent files:", Defined::DefaultFont, sf::Color(11, 129, 133, 255)),
			_newButton(sf::Vector2f(200.0, 40.0), sf::Vector2f(50.0, 50.0), 18, "Create New Atlas", ActionEvent::BUTTON_CLICKED_1, Defined::DefaultFont, TextAlignment::Left, sf::Color(40, 40, 40, 255)),
			_openButton(sf::Vector2f(200.0, 40.0), sf::Vector2f(50.0, 120.0), 18, "Open Existing Atlas", ActionEvent::BUTTON_CLICKED_2, Defined::DefaultFont, TextAlignment::Left, sf::Color(40, 40, 40, 255)),
			_enterNameTextBox(sf::Vector2f(350.0, 50.0), 14, "New Atlas Name: ", Defined::DefaultFont, sf::Color(180, 180, 180, 255)),
			_nameBox(sf::Vector2f(400.0, 50.0), sf::Vector2f(350.0, 70.0), 16, "NewAtlas", ActionEvent::INPUTBOX_ENTER_1, Defined::DefaultFont, Defined::AlphaNumeric, Defined::DarkGrey),
			_tickBox(sf::Vector2f(20.0, 20.0), sf::Vector2f(350.0, 130.0), ActionEvent::TICKBOX_MODIFIED_1, false, Defined::DarkGrey),
			_continueButton(sf::Vector2f(100.0, 40.0), sf::Vector2f(1050.0, 720.0), 18, "Continue", ActionEvent::BUTTON_CLICKED_3, Defined::DefaultFont, TextAlignment::Center, Defined::DarkCyan),
			_intBox(sf::Vector2f(60.0, 40.0), sf::Vector2f(300.0, 170.0), 16, 0, 255, ActionEvent::INPUTBOX_ENTER_2, Defined::DefaultFont, Defined::DarkGrey, sf::Color(255, 255, 0, 255)),
			_colBox(50.0, sf::Vector2f(400.0, 170.0), &_intBox),
			_openFileDialog(_window.getSystemHandle())
		{}

		ActionResult run()
		{
			_loadRecentButtons();
			while (true)
			{
				if (_openFileDialog.isStarted() && _openFileDialog.isReady())
				{
					_openButton.setSelected(false);
					if (_openFileDialog.getPath().size())
					{
						return ActionResult(ActionEvent::CONTINUE_2, util::toString(_openFileDialog.getPath()));
					}
				}

				const ActionResult responseEvent = _handleEvents();
				if(responseEvent == ActionEvent::WINDOW_CLOSED || responseEvent == ActionEvent::CONTINUE_1 || responseEvent == ActionEvent::CONTINUE_2)
					return responseEvent;

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
			run();
		}
		
		void run()
		{
			while (_window.isOpen())
			{
				ActionResult returnEvent = _startMenu.run();
				if (returnEvent == ActionEvent::WINDOW_CLOSED)
					_window.close();
				else if (returnEvent == ActionEvent::CONTINUE_1)
				{
					_editor.addAtlas(Atlas(returnEvent.obj));
					_editor.run();
				}
				else if (returnEvent == ActionEvent::CONTINUE_2)
				{
					_editor.addAtlas(std::move(Atlas::loadFromFile(returnEvent.obj)));
					_editor.run();
				}
			}
		}
	};
}

int main()
{
	if (ui::Defined::load())
	{
		ui::UIDriver driver(sf::VideoMode(1200, 800));
		std::cout << img::ImageBox::instanceCount;
		//getchar();
	}
	else
	{
		std::cout << "Failed to load!\n";
		return 0;
	}

	ui::Defined::release();
	return 0;
}
