#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include <cassert>

enum class color: uint8_t {
	WHITE, BLACK
};
enum class dir  : uint8_t {
	RIGHT, LEFT, CENTER
};

struct Piece {
	color c : 1;
	Piece(color c_) : c(c_) {}
	Piece(const Piece& p) : c(p.c) {}

	void draw(olc::PixelGameEngine* pge, olc::vi2d pos, olc::vi2d size, bool selected = false) {
		olc::Pixel c_ = olc::YELLOW;
		if (!selected) c_ = (c == color::WHITE) ? olc::WHITE : olc::BLACK;
		float x = (pos.y % 2) ? (float)pos.x * 2.0f : (float)pos.x  * 2.0f + 1.0f;
		pge->FillCircle(
			int32_t((x + 0.5f) * size.x),
			int32_t(((float)pos.y + 0.5f) * size.y),
			int32_t(std::min(size.x, size.y) * 0.4f), c_);
	}
};

struct Move {
	std::vector<olc::vi2d> path, eaten;
	Move(olc::vi2d in) {
		path.push_back(in);
	}
	Move(const Move& _Move) : path(_Move.path), eaten(_Move.eaten) {}
	Move(Move _Move, olc::vi2d pos) : path(_Move.path), eaten(_Move.eaten) {
		path.push_back(pos);
	}
};

auto x_posToBoard = [](olc::vi2d p) {return (p.y % 2) ? p.x * 2 : p.x * 2 + 1; };
auto x_boardToPos = [](olc::vi2d p) {return (p.y % 2) ? p.x / 2 : (p.x - 1) / 2; };
// check if element in vector
template<typename T>
bool isin(std::vector<T> v, T t) {
	for (T test : v)
		if (test == t) return true;
	return false;
}
// Adds to vectors
template<typename T>
std::vector<T> add(std::vector<T>a, std::vector<T>b) {
	std::vector<T> out = a;
	for (T c : b) out.push_back(c);
	return out;
}
// returns the direction of a - b
dir diffDir(olc::vi2d a, olc::vi2d b) {
	if (a.y - b.y < 0) return diffDir(b, a);


	int a_ = x_posToBoard(a);
	int b_ = x_posToBoard(b);
	if (a_ == b_) return dir::CENTER;
	return (a_ - b_ > 0) ? dir::LEFT : dir::RIGHT;
}

// get the tile of the next row
olc::vi2d* getNext(olc::vi2d pos, dir d, color c) {
	if (d == dir::CENTER) return nullptr;
	if (c == color::WHITE && pos.y == 0) return nullptr;
	if (c == color::BLACK && pos.y == 7) return nullptr;

	olc::vi2d* out;
	if (c == color::WHITE) {
		if (pos.y % 2) out = (d == dir::LEFT) ? new olc::vi2d{ pos.x - 1, pos.y - 1 } : new olc::vi2d{ pos.x, pos.y - 1 };
		else		   out = (d == dir::LEFT) ? new olc::vi2d{ pos.x, pos.y - 1 } : new olc::vi2d{ pos.x + 1, pos.y - 1 };
	}
	else {
		if (pos.y % 2) out = (d == dir::LEFT) ? new olc::vi2d{ pos.x, pos.y + 1 } : new olc::vi2d{ pos.x - 1, pos.y + 1 };
		else		   out = (d == dir::LEFT) ? new olc::vi2d{ pos.x + 1, pos.y + 1 } : new olc::vi2d{ pos.x, pos.y + 1 };
	}

	
	//out = new olc::vi2d(temp + olc::vi2d{ 0, (c == color::BLACK) ? 1 : -1 });
	if (out->x < 0 || out->x > 3) {
		delete out;
		return nullptr;
	}
	return out;
}

// reoving the first appearance of an item from a vector
template <typename T>
void remove(std::vector<T>& v, T t) {
	size_t i;
	for (i = 0; i < v.size(); i++) {
		if (v[i] == t) break;
	}
	v.erase(v.begin() + i);
}

// returns the index of the biggest value
size_t max(std::vector<size_t> in) {
	size_t max_i = 0, max_val = 0;
	for (size_t i = 0; i < in.size(); i++) {
		if (in[i] > max_val) {
			max_val = in[i];
			max_i = i;
		}
	}
	return max_i;
}



// Override base class with your custom functionality
class Checkers : public olc::PixelGameEngine
{
public:
	Checkers()
	{
		sAppName = "Checkers";
	}
private:

	Piece* board[4][8] = {nullptr};

	// MetaData - stores the places of the whites \ blacks on the board
	std::vector<olc::vi2d> vWhite, vBlack;
	
	olc::vi2d cellSize, * selected;
	color turn;


	void newBoard() {
		for (uint8_t x = 0; x < 4; x++)
			for (uint8_t y = 0; y < 8; y++)
				if (board[x][y]) {
					delete board[x][y];
					board[x][y] = nullptr;
				}

		vWhite.clear(); vWhite.resize(0);
		vBlack.clear(); vBlack.resize(0);

		for (uint8_t x = 0; x < 4; x++) {
			for (uint8_t y = 0; y < 3; y++) {
				if (y == 1 && x == 2) continue;
				if (y == 1 && x == 3) continue;
				if (y == 1 && x == 1) continue;
				board[x][y] = new Piece(color::BLACK);
				vBlack.push_back(olc::vi2d{ x,y });
			}
			for (uint8_t y = 5; y < 8; y++) {
				board[x][y] = new Piece(color::WHITE);
				vWhite.push_back(olc::vi2d{ x,y });
			}
		}
		board[2][4] = new Piece(color::BLACK);
		vBlack.push_back(olc::vi2d{ 2,4 });

	}

	// returns the players available moves by pos
	std::vector<Move> getMoves(Move pos, color c, bool isRoot = true) {
		olc::vi2d last = pos.path.back();
		if ((last.y == 0 && c == color::WHITE) ||
			(last.y == 7 && c == color::BLACK) ||
			last.x <  0 ||
			last.x >  3) return std::vector<Move>();

		std::vector<olc::vi2d> _out;
		olc::vi2d* temp = getNext(last, dir::RIGHT, c);
		if (temp) {
			_out.push_back(*temp);
			delete temp;
		}
		temp = getNext(last, dir::LEFT, c);
		if (temp) {
			_out.push_back(*temp);
			delete temp;
		}
		
		std::vector<Move> out;
		for (olc::vi2d p : _out) {
			if ((isin(vWhite, p) && c == color::WHITE) ||
				(isin(vBlack, p) && c == color::BLACK)) continue;

			if ((isin(vBlack, p) && c == color::WHITE) || 
				(isin(vWhite, p) && c == color::BLACK)) {
				dir d = diffDir(last, p);
				temp = getNext(p, d, c);

				if (temp == nullptr) continue;
				if (!isin(add(vBlack, vWhite), *temp)) {
					Move m(pos, *temp);
					m.eaten.push_back(p);
					out.push_back(m);
					out = add(out, getMoves(m, c, false));
				}
				delete temp;
				continue;
			}
			if (isRoot) out.push_back(Move(pos, p));
		}
		return out;
	}
	
	
	void doWhiteMove(Move m) {

		// changing piece pos
		board[m.path.back().x][m.path.back().y] = board[m.path[0].x][m.path[0].y];
		board[m.path[0].x][m.path[0].y] = nullptr;

		vWhite.push_back(m.path.back());
		remove(vWhite, m.path[0]);

		for (olc::vi2d p : m.eaten) {
			board[p.x][p.y] = nullptr;
			remove(vBlack, p);
		}

		// change turn
		turn = color::BLACK;

		// garbage collector
		delete selected;
		selected = nullptr;
	}

	void doBlackMove(Move m) {
		// changing piece pos
		board[m.path.back().x][m.path.back().y] = board[m.path[0].x][m.path[0].y];
		board[m.path[0].x][m.path[0].y] = nullptr;

		vBlack.push_back(m.path.back());
		remove(vBlack, m.path[0]);

		for (olc::vi2d p : m.eaten) {
			board[p.x][p.y] = nullptr;
			remove(vWhite, p);
		}
		turn = color::WHITE;
	}
	int evalBlackMove(Move m) {
		//		how far you went
		int out = (m.path.back().y - m.path[0].y);
		//	   5 * how much you eat
		out += 5 * m.eaten.size();
		//		how far are you
		out += m.path.back().y;
		// you wouldn't want to move your first row
		if (m.path[0].y == 0) out -= 6;
		return out;
		//return m.eaten.size();
	}
public:
	
	bool OnUserCreate() override
	{
		for (uint8_t x = 0; x < 4; x++)
			for (uint8_t y = 0; y < 8; y++)
				board[x][y] = nullptr;

		cellSize = {ScreenWidth() >> 3, ScreenHeight() >> 3};
		selected = nullptr;
		turn = color::WHITE;
		newBoard();

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		if (GetKey(olc::ESCAPE).bPressed) return false;

		// Black Move
		if (turn == color::BLACK) {

			int bestScore=0, temp;
			Move* bestMove = nullptr;
			for (olc::vi2d p : vBlack) {
				for (Move m : getMoves(p, color::BLACK)) {
					temp = evalBlackMove(m);
					if (temp > bestScore || !bestMove) {
						if (bestMove) delete bestMove;
						bestMove = new Move(m);
						bestScore = temp;
					}
				}
			}

			if (!bestMove) {
				std::cout << "White wins\n";
				return false;
			}
			selected = nullptr;
			doBlackMove(*bestMove);
		}

		Clear(olc::DARK_CYAN);
		// draw squares
		for (uint8_t y = 0; y < 8; y++)
			for (uint8_t x = 0; x < 8; x++)
				if ((x + y) % 2 == 0)
					FillRect(olc::vi2d{ x , y } *cellSize, cellSize, olc::GREY);

		// White Move
		if (selected)
			for (Move m : getMoves(*selected, color::WHITE)) {
				uint8_t x = x_posToBoard(m.path.back());
				FillRect(olc::vi2d{ x , m.path.back().y } *cellSize, cellSize, olc::YELLOW);
				if (GetMousePos() / cellSize == olc::vi2d{ x , m.path.back().y }) {
					for (olc::vi2d red : m.eaten)
						FillRect(olc::vi2d{ x_posToBoard(red) , red.y } *cellSize, cellSize, olc::RED);

					if (GetMouse(0).bPressed) doWhiteMove(m);
				}
			}

		// draw pieces
		for (olc::vi2d p : add(vBlack, vWhite))
			board[p.x][p.y]->draw(this, { p.x,p.y }, cellSize, selected ? olc::vi2d{ p.x,p.y } == *selected : false);


		// Get selected
		if (GetMouse(0).bPressed) {
			if (!selected) delete selected;
			olc::vi2d* temp = new olc::vi2d(GetMouseX() / cellSize.x, GetMouseY() / cellSize.y);
			selected = new olc::vi2d(x_boardToPos(*temp), temp->y);
			delete temp;

			if (!isin(vWhite, *selected)) {
				delete selected;
				selected = nullptr;
			}
		}
		return true;
	}
};

int main()
{
	uint8_t res = 1;
	Checkers demo;
	if (demo.Construct(256 << res, 240 << res, 4 >> res, 4 >> res))
		demo.Start();
	return 0;
}