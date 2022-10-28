#include <SFML/Graphics.hpp>
#include <iostream>
#include "dirent.h"
#include <filesystem>

namespace img
{
	sf::Vector2f operator* (const sf::Vector2f first, const sf::Vector2f second) { return sf::Vector2f(first.x * second.x, first.y * second.y); }

	class ImageBox
	{
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

		inline sf::Vector2f getPosition() { return _sp.getPosition(); }

		inline void setPosition(sf::Vector2f position) { _sp.setPosition(position);}
		inline void move(sf::Vector2f delta) { _sp.move(delta); }
		inline void setScale(sf::Vector2f scale) { _sp.setScale(scale); }
		inline void applyScale(sf::Vector2f scale) { _sp.setScale(_sp.getScale() * scale); }

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

		inline void setScale(sf::Vector2f scale)
		{
			_scale = scale;
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->setScale(scale);
		}
		inline void setPosition(sf::Vector2f position)
		{
			const sf::Vector2f delta = position - _position;
			_position = position;
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->move(delta);
		}

		inline void applyScale(sf::Vector2f scale)
		{
			_scale = _scale * scale;
			for (size_t i = 0; i < _images.size(); ++i)
				_images[i]->applyScale(scale);
		}

		inline void draw(sf::RenderWindow& window)
		{
			for (size_t i = 0; i < _images.size(); ++i)
				window.draw(_images[i]->getDrawObject());
		}
	};
}

namespace ui
{
	using namespace img;

	
}

int main()
{
	std::cout << sizeof(img::ImageBox);
	sf::RenderWindow window(sf::VideoMode(1200, 1200), "SFML works!");
	{
		img::ImageVector canvas;
		img::ImageVector canvas2;
		canvas2.setPosition(sf::Vector2f(0.0, 0.0));

		while (window.isOpen())
		{
			std::string input;
			getline(std::cin, input);
			if (input[0] == 'x') break;
			//if (input[0] == 'm') 
			canvas.addImage(input);
			canvas.applyScale(sf::Vector2f(0.95, 0.95));
			canvas2.mergeWith(std::move(canvas));

			sf::Event event;
			while (window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed)
					window.close();
			}




			window.clear();

			canvas.draw(window);
			canvas2.draw(window);

			window.display();
		}
	}
	std::cout << img::ImageBox::instanceCount;
	getchar();

	return 0;
}
