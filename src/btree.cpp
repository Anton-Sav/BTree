////////////////////////////////////////////////////////////////////////////////
// Module Name:  btree.h/cpp
// Authors:      Sergey Shershakov
// Version:      0.1.0
// Date:         01.05.2017
//
// This is a part of the course "Algorithms and Data Structures" 
// provided by  the School of Software Engineering of the Faculty 
// of Computer Science at the Higher School of Economics.
////////////////////////////////////////////////////////////////////////////////


#include "btree.h"

#include <stdexcept>        // std::invalid_argument
#include <cstring>          // memset


namespace xi
{


//==============================================================================
// class BaseBTree
//==============================================================================



bool BaseBTree::Header::checkIntegrity()
{
    return (sign == VALID_SIGN) && (order >= 1) && (recSize > 0);
}


BaseBTree::BaseBTree(UShort order, UShort recSize, IComparator *comparator, std::iostream *stream)
        : _order(order),
          _recSize(recSize),
          _comparator(comparator),
          _stream(stream),
          _lastPageNum(0),
          _rootPageNum(0), _rootPage(this)
{
}


BaseBTree::BaseBTree(IComparator *comparator, std::iostream *stream) :
        BaseBTree(
                0,      // порядок, 0 — д.б. прочитан из файла!
                0,      // размер ключа, 0 —  --//--
                comparator, stream)
{

}


BaseBTree::~BaseBTree()
{
}


void BaseBTree::resetBTree()
{
    _order = 0;
    _recSize = 0;
    _stream = nullptr;
    _comparator = nullptr;      // для порядку его тоже сбасываем, но это не очень обязательно
}


void BaseBTree::readPage(UInt pnum, Byte *dst)
{
    checkForOpenStream();
    if (pnum == 0 || pnum > getLastPageNum())
        throw std::invalid_argument("Can't read a non-existing page");

    readPageInternal(pnum, dst);
}


void BaseBTree::writePage(UInt pnum, const Byte *dst)
{
    checkForOpenStream();

    if (pnum == 0 || pnum > getLastPageNum())
        throw std::invalid_argument("Can't write a non-existing page");

    writePageInternal(pnum, dst);

}


bool BaseBTree::checkKeysNumber(UShort keysNum, bool isRoot)
{
    if (keysNum > getMaxKeys())
        return false;                       // превышение по максимуму

    // NOTE: для корня пока даже 0 допустим, потом уточним, надо ли до 1 сокращать
    if (isRoot)
        //if (nt == nRoot)
        return true;

    return (keysNum >= getMinKeys());
}


void BaseBTree::checkKeysNumberExc(UShort keysNum, bool isRoot)
{
    if (!checkKeysNumber(keysNum, isRoot))
        throw std::invalid_argument("Invalid number of keys for a node");
}


UInt BaseBTree::allocPage(PageWrapper &pw, UShort keysNum, bool isLeaf /*= false*/)
{
    checkForOpenStream();
    checkKeysNumberExc(keysNum, pw.isRoot());  // nt);

    return allocPageInternal(pw, keysNum, pw.isRoot(), isLeaf);
}


xi::UInt BaseBTree::allocNewRootPage(PageWrapper &pw)
{
    checkForOpenStream();
    return allocPageInternal(pw, 0, true, false);
}


void BaseBTree::insert(const xi::Byte *k)
{

    // TODO: релаизовать студентам!

    if (k == nullptr)
        return;;
    //create a new root to insert
    UInt newRoot = _rootPageNum;

    //check if the root is full
    if (_rootPage.isFull())
    {

        _rootPage.allocNewRootPage(); //distribute the current page under the new root

        _rootPage.setCursor(0, newRoot); //set the cursor for the new root

        _rootPageNum = _rootPage.getPageNum(); //write the new page number as root

        _rootPage.splitChild(0); //split

        _rootPage.readPage(_rootPageNum); //then read the contents of root

        _rootPage.insertNonFull(k); //insert item into split root

    } else
    {
        _rootPage.insertNonFull(k); //if the root is not full then simply insert the element
    }

}

Byte *BaseBTree::search(const Byte *k)
{
    // TODO: релаизовать студентам!

    PageWrapper currentPage(this); //create a new object to write page data

    currentPage.readPage(_rootPageNum); //start the search from the root, read data from it

    UShort key; //key storage variable

    //a cycle in order to search for elements, we will leave as soon as we reach the end of the tree,
    //or we will return the key, or nullptr if it is not found
    while (true)
    {
        key = currentPage.getKeysNum(); //remember the key of the current page

        UShort i = 0; //create a counter so that it is visible outside the nested loop while

        //go over the page and look for the item you want
        while (i < key && _comparator->compare(currentPage.getKey(i), k, _recSize))
        {
            //if the item is found then return it
            if (i < key && _comparator->isEqual(k, currentPage.getKey(i), _recSize))
                return new Byte(*currentPage.getKey(i));

            ++i;
        }

        //if the page has no descendants, return nullptr
        if (currentPage.isLeaf())
            return nullptr;
        else
            currentPage.readPageFromChild(currentPage, i); //otherwise look for an element in the descendants

    }
}

int BaseBTree::searchAll(const Byte *k, std::list<Byte *> &keys)
{
    // TODO: релаизовать студентам!   

    _rootPage.readPage(_rootPageNum); //start the search from the root, read data from it

    int needKey = _rootPage.searchAll(k, keys); //start the search but from the object PageWrapper

    return needKey;
}

int BaseBTree::PageWrapper::searchAll(const Byte *k, std::list<Byte *> &keys)
{
    // TODO: релаизовать студентам!

    PageWrapper currentPage(_tree); //create a new page for working with wood
    int counNeedElement = 0;    //counter to count the number of items sought

    UShort key = getKeysNum(); //remember the current key

    UShort i = 0; //create a counter
    //run up the page to find the highest index of the desired item
    while (i < key && _tree->_comparator->compare(getKey(i), k, _tree->_recSize))
        ++i;

    //if our node is not a leaf, then go to its leftmost descendant
    if (!isLeaf())
    {
        currentPage.readPageFromChild(*this, i); //read current page

        //recursively go to the end of the tree to the sheets
        counNeedElement += currentPage.searchAll(k, keys);
    }

    //go over the page to find all the desired items, if they exist
    while (i < key && _tree->_comparator->isEqual(k, getKey(i), _tree->_recSize))
    {
        //if found, add its key to the list, and increase the counter
        ++counNeedElement;
        keys.push_back(new Byte(*getKey(++i)));

        //check the right subtree like the left
        if (!isLeaf())
        {
            currentPage.readPageFromChild(*this, i); //read current page
            counNeedElement += currentPage.searchAll(k, keys); //recursively go to the end of the tree to the sheets
        }
    }

    return counNeedElement;
}

//UInt BaseBTree::allocPageInternal(UShort keysNum, NodeType nt, PageWrapper& pw)
UInt BaseBTree::allocPageInternal(PageWrapper &pw, UShort keysNum, bool isRoot, bool isLeaf)
{
    // подготовим страничку для вывода
    pw.clear();
    pw.setKeyNumLeaf(keysNum, isRoot, isLeaf);    // nt);

    // пока что просто позиционируемся в конец, считая, что так и будет, но держать под контролем
    _stream->seekg(0, std::ios_base::end);
    _stream->write((const char *) pw.getData(), getNodePageSize());

    ++_lastPageNum;
    writePageCounter();


    // TODO: flush-им на диск сразу?!
    //_stream->flush();

    return _lastPageNum;
}


void BaseBTree::readPageInternal(UInt pnum, Byte *dst)
{
    // позиционируемся и читаем
    gotoPage(pnum);
    _stream->read((char *) dst, getNodePageSize());
}


void BaseBTree::writePageInternal(UInt pnum, const Byte *dst)
{
    // позиционируемся и пишем
    gotoPage(pnum);
    _stream->write((const char *) dst, getNodePageSize());
}


void BaseBTree::gotoPage(UInt pnum)
{
    // рассчитаем смещение до нужной страницы
    UInt pageOfs = FIRST_PAGE_OFS + getNodePageSize() * (pnum - 1);     // т.к. нумеруются с единицы
    _stream->seekg(pageOfs, std::ios_base::beg);
}


void BaseBTree::loadTree()
{
    // _stream->seekg(0, std::ios_base::beg);       // пока загружаем с текущего места в потоке!
    // читаем заголовок

    Header hdr;
    readHeader(hdr);

    // если при чтении случилась пичалька
    if (_stream->fail())
    {
        //_stream->close();
        throw std::runtime_error("Can't read header");
    }

    // проверяет заголовок на корректность
    if (!hdr.checkIntegrity())
    {
        //_stream->close();
        throw std::runtime_error("Stream is not a valid xi B-tree file");
    }

    // задаем порядок и т.д.
    setOrder(hdr.order, hdr.recSize);

    // далее без проверки читаем два следующих поля
    readPageCounter();             // номер текущей свободной страницы
    readRootPageNum();             // номер корневой страницы

    // если при чтении случилась пичалька
    if (_stream->fail())
    {
        //_fileStream.close();
        throw std::runtime_error("Can't read necessary fields. File corrupted");
    }

    // загрузить корневую страницу
    loadRootPage();

}


void BaseBTree::loadRootPage()  //PageWrapper& pw)
{
    if (getRootPageNum() == 0)
        throw std::runtime_error("Root page is not defined");

    _rootPage.readPage(getRootPageNum());
    _rootPage.setAsRoot(false);               // в файл номер страницы не пишем, т.к. только что прочитали оттуда его

}


void BaseBTree::createTree(UShort order, UShort recSize)
{
    setOrder(order, recSize);

    writeHeader();                  // записываем заголовок файла
    writePageCounter();             // и номер текущей свободной страницы
    writeRootPageNum();             // и номер корневой страницы


    // создать корневую страницу
    createRootPage();
}


void BaseBTree::createRootPage()
{
    _rootPage.allocPage(0, true);
    _rootPage.setAsRoot();
}


void BaseBTree::checkForOpenStream()
{
    if (!isOpen())
        throw std::runtime_error("Stream is not ready");

}


void BaseBTree::writeHeader()
{
    Header hdr(_order, _recSize);
    _stream->write((const char *) (void *) &hdr, HEADER_SIZE);

}

void BaseBTree::readHeader(Header &hdr)
{
    _stream->seekg(HEADER_OFS, std::ios_base::beg);
    _stream->read((char *) &hdr, HEADER_SIZE);

}


void BaseBTree::writePageCounter() //UInt pc)
{
    _stream->seekg(PAGE_COUNTER_OFS, std::ios_base::beg);
    _stream->write((const char *) &_lastPageNum, PAGE_COUNTER_SZ);
}


//xi::UInt
void BaseBTree::readPageCounter()
{
    _stream->seekg(PAGE_COUNTER_OFS, std::ios_base::beg);
    _stream->read((char *) &_lastPageNum, PAGE_COUNTER_SZ);
}


void BaseBTree::writeRootPageNum() //UInt rpn)
{
    _stream->seekg(ROOT_PAGE_NUM_OFS, std::ios_base::beg);
    _stream->write((const char *) &_rootPageNum, ROOT_PAGE_NUM_SZ);

}


//xi::UInt
void BaseBTree::readRootPageNum()
{
    _stream->seekg(ROOT_PAGE_NUM_OFS, std::ios_base::beg);
    _stream->read((char *) &_rootPageNum, ROOT_PAGE_NUM_SZ);
}


void BaseBTree::setRootPageNum(UInt pnum, bool writeFlag /*= true*/)
{
    _rootPageNum = pnum;
    if (writeFlag)
        writeRootPageNum();
}


void BaseBTree::setOrder(UShort order, UShort recSize)
{
    // метод закрытый, корректность параметров должно проверять в вызывающих методах

    _order = order;
    _recSize = recSize;

    _minKeys = order - 1;
    _maxKeys = 2 * order - 1;

    // проверим максимальное число ключей
    if (_maxKeys > MAX_KEYS_NUM)
        throw std::invalid_argument("For a given B-tree order, there is an excess of the maximum number of keys");

    _keysSize = _recSize * _maxKeys;                // область памяти под ключи
    _cursorsOfs = _keysSize + KEYS_OFS;             // смещение области курсоров на дочерние
    _nodePageSize = _cursorsOfs + CURSOR_SZ * (2 * order);  // размер узла целиком, опр. концом области страницы

    // Q: номер текущей корневой надо устанавливать?

    // пока-что распределяем память под рабочую страницу/узел здесь, но это сомнительно
    reallocWorkPages();
}


void BaseBTree::reallocWorkPages()
{
    _rootPage.reallocData(_nodePageSize);
}


//==============================================================================
// class BaseBTree::PageWrapper
//==============================================================================



BaseBTree::PageWrapper::PageWrapper(BaseBTree *tr) :
        _data(nullptr), _tree(tr), _pageNum(0)
{
    // если к моменту создания странички дерево уже в работе (открыто), надо
    // сразу распределить память!
    if (_tree->isOpen())
        reallocData(_tree->getNodePageSize());

}


BaseBTree::PageWrapper::~PageWrapper()
{
    reallocData(0);
}


void BaseBTree::PageWrapper::reallocData(UInt sz)
{
    if (_data)
        delete[] _data;

    if (sz)
        _data = new Byte[sz];

}

void BaseBTree::PageWrapper::clear()
{
    if (!_data)
        return;

    // работая с сырыми блоками данных единственно и можно применять низкоуровневые C-функции
    memset(_data, 0, _tree->getNodePageSize());
}


void BaseBTree::PageWrapper::setKeyNumLeaf(UShort keysNum, bool isRoot, bool isLeaf) //NodeType nt)
{
    _tree->checkKeysNumberExc(keysNum, isRoot);

    // логически приплюсовываем
    if (isLeaf)
        keysNum |= LEAF_NODE_MASK; // 0x8000;

    // способ записи типизированного объекта, начиная с адреса [0]
    *((UShort *) &_data[0]) = keysNum;
}


void BaseBTree::PageWrapper::setKeyNum(UShort keysNum, bool isRoot) //NodeType nt)
{
    _tree->checkKeysNumberExc(keysNum, isRoot);


    UShort kldata = *((UShort *) &_data[0]);      // взяли сущ. значение
    kldata &= LEAF_NODE_MASK;                   // оставили от него только флаг "лист"
    kldata |= keysNum;                          // приилили число ключей (там точно не будет 1 в старшем)

    *((UShort *) &_data[0]) = kldata;             // записали
}


void BaseBTree::PageWrapper::setLeaf(bool isLeaf)
{
    UShort kldata = *((UShort *) &_data[0]);      // взяли сущ. значение
    kldata &= ~LEAF_NODE_MASK;                  // оставили от него только часть с числом ключей
    if (isLeaf)
        kldata |= LEAF_NODE_MASK;   // 0x8000;  // приилили флаг, если надо

    *((UShort *) &_data[0]) = kldata;             // записали
}


bool BaseBTree::PageWrapper::isLeaf() const
{
    UShort info = *((UShort *) &_data[0]);

    return (info & LEAF_NODE_MASK) != 0;
}

UShort BaseBTree::PageWrapper::getKeysNum() const
{
    UShort info = *((UShort *) &_data[0]);

    return (info & ~LEAF_NODE_MASK);
}


Byte *BaseBTree::PageWrapper::getKey(UShort num)
{
    // рассчитываем смещение
    //UInt kofst = KEYS_OFS + _tree->getRecSize() * num;
    int kofst = getKeyOfs(num);
    if (kofst == -1)
        return nullptr;

    return (_data + kofst);
}


const Byte *BaseBTree::PageWrapper::getKey(UShort num) const
{
    int kofst = getKeyOfs(num);
    if (kofst == -1)
        return nullptr;

    return (_data + kofst);
}


void BaseBTree::PageWrapper::copyKey(Byte *dst, const Byte *src)
{
    memcpy(
            dst,                        // куда
            src,                        // откуда
            _tree->getRecSize());       // размер ключа
}

void BaseBTree::PageWrapper::copyKeys(Byte *dst, const Byte *src, UShort num)
{
    memcpy(
            dst,                        // куда
            src,                        // откуда
            _tree->getRecSize() * num   // размер: размер записи на число элементов
    );

}

void BaseBTree::PageWrapper::copyCursors(Byte *dst, const Byte *src, UShort num)
{
    memcpy(
            dst,                        // куда
            src,                        // откуда
            num * CURSOR_SZ             // размер
    );
}

UInt BaseBTree::PageWrapper::getCursor(UShort cnum)
{
    //if (cnum > getKeysNum())
    int curOfs = getCursorOfs(cnum);
    if (curOfs == -1)
        throw std::invalid_argument("Wrong cursor number");

    return *((const UInt *) (_data + curOfs));
}


Byte *BaseBTree::PageWrapper::getCursorPtr(UShort cnum)
{
    int curOfs = getCursorOfs(cnum);
    if (curOfs == -1)
        throw std::invalid_argument("Wrong cursor number");

    return (_data + curOfs);

}

void BaseBTree::PageWrapper::setCursor(UShort cnum, UInt cval)
{
    int curOfs = getCursorOfs(cnum);
    if (curOfs == -1)
        throw std::invalid_argument("Wrong cursor number");

    *((UInt *) (_data + curOfs)) = cval;
}


int BaseBTree::PageWrapper::getCursorOfs(UShort cnum) const
{
    if (cnum > getKeysNum())
        return -1;

    // рассчитываем смещением
    return _tree->getCursorsOfs() + CURSOR_SZ * cnum;
}

int BaseBTree::PageWrapper::getKeyOfs(UShort num) const
{
    if (num >= getKeysNum())
        return -1;

    // рассчитываем смещение
    return KEYS_OFS + _tree->getRecSize() * num;
}


void BaseBTree::PageWrapper::setAsRoot(bool writeFlag /*= true*/)
{
    _tree->_rootPageNum = _pageNum;         // ид корень по номеру страницы в памяти

    if (!writeFlag)
        return;

    // если же надо записать в файл сразу...
    if (_pageNum == 0)
        throw std::runtime_error("Can't set a page as root until allocate a page in a file");

    // если же под страницу есть номер, запишем его в файл
    _tree->setRootPageNum(_pageNum, true);
}


void BaseBTree::PageWrapper::readPageFromChild(PageWrapper &pw, UShort chNum)
{
    UInt cur = pw.getCursor(chNum);
    if (cur == 0)
        throw std::invalid_argument("Cursor does not point to a existing node/page");

    readPage(cur);
}


void BaseBTree::PageWrapper::writePage()
{
    if (getPageNum() == 0)
        throw std::runtime_error("Page number not set. Can't write");

    _tree->writePage(getPageNum(), _data);
}


void BaseBTree::PageWrapper::splitChild(UShort iChild)
{
    if (isFull())
        throw std::domain_error("A parent node is full, so its child can't be splitted");

    if (iChild > getKeysNum())
        throw std::invalid_argument("Cursor not exists");


    // TODO: реализовать студентам

    PageWrapper left(_tree); //create objects to divide the page into 2 children

    PageWrapper right(_tree); //create objects to divide the page into 2 children

    left.readPageFromChild(*this, iChild); //write the object to the left

    //put the right page in the queue for the left and associate Wrapper with it
    right.allocPage((UShort) _tree->_minKeys, left.isLeaf());

    //we copy all data of the right page
    right.copyKeys(right.getKey(0), left.getKey((UShort) (_tree->_minKeys + 1)), (UShort) _tree->_minKeys);
    //if it is not a sheet, we do similar actions with descendants
    if (!left.isLeaf())
        right.copyCursors(right.getCursorPtr(0), left.getCursorPtr((UShort) (_tree->_minKeys + 1)),
                          (UShort) (_tree->_minKeys + 1));

    //make room for a new item
    setKeyNum((UShort) (getKeysNum() + 1)); //raise the central element up

    UShort currentKey = getKeysNum(); //read current key

    //loop to insert an element into the top node
    while (currentKey > (UShort) (iChild + 1))
    {
        copyCursors(getCursorPtr(currentKey), getCursorPtr((UShort) (currentKey - 1)), 1);
        --currentKey;
    }
    setCursor((UShort) (iChild + 1), right.getPageNum());


    //similar actions: the nose is already with the keys
    UShort currentKeyRig = (UShort) (getKeysNum() - 1);
    while (currentKeyRig > iChild)
    {
        copyKey(getKey(currentKeyRig), getKey((UShort) (currentKeyRig - 1)));
        --currentKeyRig;
    }
    copyKey(getKey(iChild), left.getKey((UShort) _tree->_minKeys));

    left.setKeyNum((UShort) _tree->_minKeys); //we cut off all unnecessary elements

    //write all changes to the file ->
    right.writePage();
    left.writePage();
    writePage();
    // <-
}


void BaseBTree::PageWrapper::insertNonFull(const Byte *k)
{
    if (isFull())
        throw std::domain_error("Node is full. Can't insert");

    IComparator *c = _tree->getComparator();
    if (!c)
        throw std::runtime_error("Comparator not set. Can't insert");

    // TODO: реализовать студентам

    if (!isLeaf()) //if the item is a not leaf
    {
        PageWrapper currentPage(_tree); //create a new page for writing

        UShort currentKey = getKeysNum(); //read current key

        //looking for a tree in which to insert an element
        while (currentKey > 0 && c->compare(k, getKey((UShort) (currentKey - 1)), _tree->_recSize))
            --currentKey;

        currentPage.readPageFromChild(*this, currentKey); //read the current page

        if (currentPage.getKeysNum() == _tree->_maxKeys) //if the root of this subtree is full then split
        {
            splitChild(currentKey); //split

            //check for: in which of the subtrees we insert a new element
            if (c->compare(getKey(currentKey), k, _tree->_recSize))
                currentPage.readPageFromChild(*this, (UShort) (currentKey + 1));
            currentPage.readPage(currentPage._pageNum);
        }

        //recursively go this way to the sheets
        currentPage.insertNonFull(k);

    } else
    {
        UShort currentKeyLeaf = getKeysNum(); //read current key

        //make room for a new item ->
        setKeyNum((UShort) (getKeysNum() + 1));
        while (currentKeyLeaf > 0 && c->compare(k, getKey((UShort) (currentKeyLeaf - 1)), _tree->_recSize))
        {
            copyKey(getKey(currentKeyLeaf), getKey((UShort) (currentKeyLeaf - 1)));
            --currentKeyLeaf;
        }
        // <-

        copyKey(getKey(currentKeyLeaf), k); //insert item

        writePage(); //write changes to the file
    }
}




//==============================================================================
// class FileBaseBTree
//==============================================================================

FileBaseBTree::FileBaseBTree()
        : BaseBTree(0, 0, nullptr, nullptr)
{
}


FileBaseBTree::FileBaseBTree(UShort order, UShort recSize, IComparator *comparator,
                             const std::string &fileName)
        : FileBaseBTree()
{
    _comparator = comparator;

    checkTreeParams(order, recSize);
    createInternal(order, recSize, fileName);
}


FileBaseBTree::FileBaseBTree(const std::string &fileName, IComparator *comparator)
        : FileBaseBTree()
{
    _comparator = comparator;
    loadInternal(fileName); // , comparator);
}

FileBaseBTree::~FileBaseBTree()
{
    close();            // именно для удобства такого использования не кидаем внутри искл. ситуацию!
}


void FileBaseBTree::create(UShort order, UShort recSize, //IComparator* comparator,
                           const std::string &fileName)
{
    if (isOpen())
        throw std::runtime_error("B-tree file is already open");

    checkTreeParams(order, recSize);
    createInternal(order, recSize, fileName);
}


void FileBaseBTree::createInternal(UShort order, UShort recSize, // IComparator* comparator,
                                   const std::string &fileName)
{
    _fileStream.open(fileName,
                     std::fstream::in | std::fstream::out |      // чтение запись
                     std::fstream::trunc |
                     // обязательно грохнуть имеющееся (если вдруг) содержимое
                     std::fstream::binary);                      // бинарничек

    // если открыть не удалось
    if (_fileStream.fail())
    {
        // пытаемся закрыть и уходим
        _fileStream.close();
        throw std::runtime_error("Can't open file for writing");
    }

    // если же все ок, сохраняем параметры и двигаемся дальше
    //_comparator = comparator;
    _fileName = fileName;
    _stream = &_fileStream;                         // привязываем к потоку

    createTree(order, recSize);                     // в базовом дереве
}


void FileBaseBTree::open(const std::string &fileName) //, IComparator* comparator)
{
    if (isOpen())
        throw std::runtime_error("B-tree file is already open");

    loadInternal(fileName); // , comparator);
}


void FileBaseBTree::loadInternal(const std::string &fileName) // , IComparator* comparator)
{
    _fileStream.open(fileName,
                     std::fstream::in | std::fstream::out |      // чтение запись
                     //  здесь не должно быть trunc, чтобы сущ. не убить
                     std::fstream::binary);                      // бинарничек

    // если открыть не удалось
    if (_fileStream.fail())
    {
        // пытаемся закрыть и уходим
        _fileStream.close();
        throw std::runtime_error("Can't open file for reading");
    }

    // если же все ок, сохраняем параметры и двигаемся дальше
    //_comparator = comparator;
    _fileName = fileName;
    _stream = &_fileStream;         // привязываем к потоку


    try
    {
        loadTree();
    }
    catch (std::exception &e)
    {
        _fileStream.close();
        throw e;
    }
    catch (...)                     // для левых исключений
    {
        _fileStream.close();
        throw std::runtime_error("Error when loading btree");
    }
}


void FileBaseBTree::close()
{
    if (!isOpen())
        return;

    closeInternal();
}


void FileBaseBTree::closeInternal()
{
    // NOTE: возможно, перед закрытием надо что-то записать в файл? — иметь в виду!
    _fileStream.close();

    // переводим объект в состояние сконструированного БЕЗ параметров
    resetBTree();
}

void FileBaseBTree::checkTreeParams(UShort order, UShort recSize)
{
    if (order < 1 || recSize == 0)
        throw std::invalid_argument("B-tree order can't be less than 1 and record siaze can't be 0");

}

bool FileBaseBTree::isOpen() const
{
    return (_fileStream.is_open()); // && _fileStream.good());
}


} // namespace xi

