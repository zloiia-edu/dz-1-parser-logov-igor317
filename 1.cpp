/*
BD файл - DBase.ddt
Лог файл - data.txt
*/

#include <iostream>
#include <string> //подключение библиотеки для работы со строками
#include <fstream> //подключение библиотеки для работы с файлами как с потоком

using namespace std;

//коды ошибок
// 0 - нет ошибок
// 1 - ошибка открытия потока
// 2 - кончились данные

class InOut
{
protected:
	int _LastError; //значение кода последней ошибки
public:
	InOut() {
		_LastError = 0;
	}

	virtual int Read() = 0;

	virtual void Write(string txt) = 0;

	int LastError() const {
		return _LastError;
	}

	virtual void Open() = 0;
	virtual void Close() = 0;
};

class IProcess {
protected:
	InOut * _in; //указатель на входной поток ввода-вывода
	InOut *_out; //указатель на выводной поток ввода-вывода

public:
	IProcess() {
		_in = NULL;
		_out = NULL;
	}

	virtual ~IProcess() {
		if (_in != NULL)
			delete _in;
		if (_out != NULL)
			delete _out;
	}

	void SetInput(InOut* in) {
		_in = in;
	}

	void SetOutput(InOut* out) {
		_out = out;
	}

	virtual void Work() = 0;
};

class TermIO : public InOut {
public:

	TermIO() : InOut() {}

	void Open() override {
	}

	void Close() override {
	}

	int Read() override {
		int a = 0;
		string b;
		cin >> b;
		try
		{
			a = stoi(b);
		}
		catch (exception) {}

		if (a == -1) // код конца потока
		{
			_LastError = 2;
		}
		return a;
	}

	void Write(string txt) override {
		cout << txt << endl;
	}
};


class FileIO : public InOut {
private:
	string _filename; //имя файла, с которым мы будем работать
	fstream _fdesc; // дескриптор файла
	unsigned int _openmode; // Режим открытия (вынес сюда, т.к не хочу позволять записывать в файл DB и записывать в лог-файл)
public:
	FileIO(string filename, int openmode) :InOut() {
		SetFilename(filename);
		_openmode = openmode;
	}

	int Read() override {
		if (!_fdesc.is_open()) { //если файл не открыт, то нам и читать нечего
			_LastError = 1; //ставим ошибку
			return 0; //выходим
		}
		if (_fdesc.eof()) { // если конец файла - ошибка(2)
			_LastError = 2;
			return 0;
		}

		int a = 0;
		string b;
		_fdesc >> b;
		try
		{
			a = stoi(b);
		}
		catch (exception) {}

		return a;
	}

	void SetFilename(string filename) {

		_filename = filename; //сохраняем имя файла
		if (_fdesc.is_open()) { //если файл уже был открыт
			Close();  //закрываем
		}
		Open(); //открываем заново файл
	}

	void Open() override {
		_fdesc.open(_filename, _openmode); //открываем файл
	}

	void Close() override {
		_fdesc.close();
	}

	void Write(string txt) override {
		if (!_fdesc.is_open()) { //если файл не был открыт, то писать некуда
			_LastError = 1; //ставим ошибку
			return; //выходим
		}

		_fdesc << txt << endl; //пишем в файл и переводим каретку
	}

};

class DBInput{
private:
	string _dbfilename; //имя файла
	fstream _dbdesc; // дескриптор файла
public:
	DBInput(string filename)
	{
		_dbfilename = filename;
		Open();
	}
	~DBInput() {
		if (_dbdesc.is_open()) {
			Close();
		}
	}

	void Open() {
		_dbdesc.open(_dbfilename, fstream::in);
	}
	void Close() {
		_dbdesc.close();
	}

	string FindMessage(int codename) // ищем сообщение по номеру ошибки (для базы данных)
	{
		if (_dbdesc.is_open()) {
			Close();
		}
		Open();

		if (codename == -1) // Точка выхода (код для терминала)
			return "";

		string message = "";
		while (!_dbdesc.eof()) // Проверяем, не закончились ли строки
		{
			getline(_dbdesc, message); // считываем строку
			if (message == to_string(codename)) // Сопоставляем код с позицией DB
			{
				getline(_dbdesc, message); // Читаем тело 
				return message;
			}
		}
		return "Unknown error";
	}
};

class CameraProcess : public IProcess {
private:
	DBInput * _dbase; // дескриптор базы данных
public:
	CameraProcess(string dbasename) :IProcess()
	{
		_dbase = new DBInput(dbasename); // инициализация файла DB
	}
	~CameraProcess()
	{
		delete _dbase;
	}
	void Work() override {
		if (_in == NULL || _out == NULL) {
			cout << "ERROR: No input or output" << endl;
			return;
		}
		while (_in->LastError() != 2) // обрабочик
		{
			int line;
			line = _in->Read();
			if (line != 0)
				_out->Write(_dbase->FindMessage(line));
		}
		if (_in->LastError() == 2)
		{
			cout << "Messages ended" << endl;
		}
	}
};

int ConvertHandler(string value) {
	int a = 0;
	try
	{
		a = stoi(value);
	}
	catch (exception) {}
	return a;
}

int main(int argc, char** argv)
{
	int ia, oa;
	InOut* inStream = NULL;
	InOut* outStream = NULL;
	string name;
	string inputS, outputS;
	string cf;
	string tempstr;


	cout << "Choose input(Terminal(1)/File(2)): ";
	cin >> tempstr;
	ia = ConvertHandler(tempstr);

	while (ia != 1 && ia != 2) // выбираем способ ввода
	{
		cout << "Incorrect name, try again (1/2): ";
		cin >> tempstr;
		ia = ConvertHandler(tempstr);
	}
	switch (ia)
	{
	case 1:
		inStream = new TermIO(); //данные берем из терминала
		inputS = "Terminal";
		break;
	case 2:
		cout << "enter name (input): ";
		cin >> name;
		ifstream ifile(name + ".txt"); // Ищем файл
		while (!ifile) // Проверяем, существует ли файл
		{
			cout << "Incorrect name, try again (input name): ";
			cin >> name;
			ifile.open(name + ".txt");
		}
		inStream = new FileIO(name + ".txt", fstream::in); //берем из файла
		inputS = "File (" + name + ".txt)";
		break;
	}

	cout << "Choose output(Terminal(1)/File(2)): ";
	cin >> tempstr;
	oa = ConvertHandler(tempstr);

	while (oa != 1 && oa != 2) // выбираем способ вывода
	{
		cout << "incorrect value, try again (1/2): ";
		cin >> tempstr;
		oa = ConvertHandler(tempstr);
	}

	switch (oa)
	{
	case 1:
		outStream = new TermIO(); //кладем данные в терминал
		outputS = "Terminal";
		break;
	case 2:
		cout << "enter name (output): ";
		cin >> name;
		ifstream ifile(name + ".txt");

		while (ifile) // Если файл существует, спрашиваем, стоит ли использовать другой
		{
			cout << "file exists. Change file? (y/n): ";
			cin >> cf;
			//ConvertHandler(cf);
			while (cf != "y" && cf != "n")
			{
				cout << "incorrect value, try again (y/n): ";
				cin >> cf;
			}
			if (cf == "n")
			{
				outputS = "File (" + name + ".txt /exist)";
				break;
			}
			if (cf == "y")
			{
				cout << "enter name (output): ";
				cin >> name;
				ifstream ifile(name + ".txt");
				if (ifile.fail()) {
					outputS = "File (" + name + ".txt /new)";
					break;
				}
			}

		}
		if (!ifile) // если файл вывода не существует - создаем его
		{
			ofstream ofile(name + ".txt");
			outputS = "File (" + name + ".txt /new)";
		}
		outStream = new FileIO(name + ".txt", fstream::out); //кладем в файл 
		break;
	}

	cout << "Programm start. " << "input" << "(" << inputS << ")" << " || Output" << "(" << outputS << ")" << endl << endl;

	inStream->Open(); //открываем потоки
	outStream->Open();

	IProcess* process = new CameraProcess("DBase.ddt"); //создаем обработчик

	process->SetInput(inStream); //задаем потоки
	process->SetOutput(outStream);

	process->Work(); //работаем

	delete process; //удаляем объект обработчика после работы

	return 0;
}

