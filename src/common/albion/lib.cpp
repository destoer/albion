#include <albion/lib.h>

void load_ips_patch(const std::string &filename,std::vector<uint8_t> &rom)
{
	auto fp = std::fstream(filename, std::ios::in | std::ios::binary);


    if(!fp)
    {
        return;
    }

    // we have ae ips file now read it in
    std::vector<uint8_t> buf;

    fp.seekg(0,std::ios::end);
    size_t size = fp.tellg();
    fp.seekg(0,std::ios::beg);

    fp.clear();


    buf.resize(size);

    fp.read((char*)buf.data(),size);
    fp.close();


    size_t idx = 0;
    // https://zerosoft.zophar.net/ips.php
    if(size > 5)
    {
        // skip header
        idx = 5;

        while(idx < size)
        {
            // record size
            if(idx + 5 >= size)
            {
                break;
            }

            uint32_t offset = 0;

            // read out 24bit value and bswap
            memcpy(&offset,&buf[idx],3);
            unsigned char *bswap_ptr = (unsigned char *)&offset;
            std::swap(bswap_ptr[0],bswap_ptr[2]);

            idx += 3;

            // eof
            if(offset == 0x464f45)
            {
                break;
            }


            const uint16_t len = bswap(handle_read<uint16_t>(buf,idx));
            idx += 2;

            // make sure our patching wont got out of bounds
            if(len + idx >= size || offset+len >= rom.size())
            {
                break;
            }

            // patch from buffer
            if(len != 0)
            {
                // patch byte
                for(int i = 0; i < len; i++)
                {
                    rom[offset++] = buf[idx++];
                }
            }

            // RLE
            else 
            {
                // extended record
                if(idx + 3 >= size)
                {
                    break;
                }


                const uint16_t rle_len = bswap(handle_read<uint16_t>(buf,idx));
                idx += 2;
                const uint8_t data = buf[idx++];

                if(offset + rle_len >= rom.size())
                {
                    break;
                }

                for(int i = 0; i < rle_len; i++)
                {
                    rom[offset+i] = data;
                }
            }
        }
    }
    
}

// basic tokenizer
template<typename F>
bool verify_immediate_internal(const std::string &line, uint32_t &i, F lambda)
{
    const auto len = line.size();

    for(; i < len; i++)
    {
        // valid part of the value
        if(lambda(line[i]))
        {
            continue;
        }

        // values cannot have these at the end!
        else if(isalpha(line[i]))
        {
            return false;
        }

        // we have  < ; + , etc stop parsing
        else 
        {
            return true;
        }
    }

    return true;
}


bool verify_immediate(const std::string &line, std::string &literal)
{
    const auto len = line.size();

    // an empty immediate aint much use to us
    if(!len)
    {
        return false;
    }

    uint32_t i = 0;

    const auto c = line[0];

    // allow - or +
    if(c == '-' || c == '+')
    {
        i = 1;
        // no digit after the sign is of no use
        if(len == 1)
        {
            return false;
        }
    }

    bool valid = false;


    // have prefix + one more digit at minimum
    const auto prefix = i+2 < len?  line.substr(i,2) : "";

    // verify we have a valid hex number
    if(prefix == "0x")
    {
        // skip past the prefix
        i += 2;
        valid = verify_immediate_internal(line,i,[](const char c) 
        {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
    }

    // verify its ones or zeros
    else if(prefix == "0b")
    {
        // skip past the prefix
        i += 2;                
        valid = verify_immediate_internal(line,i,[](const char c) 
        {
            return c == '0' || c == '1';
        });
    }

    // verify we have all digits
    else
    {
        valid = verify_immediate_internal(line,i,[](const char c) 
        {
            return c >= '0' && c <= '9';
        });
    }
    

    if(valid)
    {
        literal = line.substr(0,i);
    }

    return valid;    
}


u32 convert_imm(const std::string &imm)
{
    try 
    {
        if(imm.size() >= 3 && imm.substr(0,2) == "0b")
        {
            return static_cast<uint32_t>(std::stoll(imm.substr(2),0,2));
        }

        // stoi wont auto detect base for binary strings?
        return static_cast<uint32_t>(std::stoll(imm,0,0));
    }

    catch(std::exception &ex)
    {
        printf("stoi exception\n");
        std::cout << ex.what() << std::endl;
        exit(1);
    }
}

bool decode_imm(const std::string &line, uint32_t &i,std::string &literal)
{
    const auto success = verify_immediate(line.substr(i),literal);

    // set one back for whatever the terminating character was
    i--;

    i += literal.size();  

    return success;
}

bool tokenize(const std::string &line,std::vector<Token> &tokens)
{
    tokens.clear();
    for(uint32_t i = 0; i < line.size(); i++)
    {
        const auto c =  line[i];
        switch(c)
        {
            case ' ': break;
            case '\n': break;
            case '\t': break;
            case '\r': break;
            case '\0': break;

            // comment end of line
            case ';': return true;


         
            default:
            {   
                token_type type;
                std::string literal = "";

                // integer
                if(isdigit(c))
                {
                    type = token_type::integer;
                    if(!decode_imm(line,i,literal))
                    {
                        return false;
                    }
                }

                // string
                else
                {
                    type = token_type::string;
                    for(; i < line.size(); i++)
                    {
                        const auto c = line[i];
                        if(c == ' ')
                        {
                            break;
                        }

                        literal += c;
                    }
                }

                // push token
                tokens.push_back(Token(literal,type));
                break;
            }
        }
    }
    return true;
}

void print_tokens(const std::vector<Token> &tokens)
{
    for(const auto &t: tokens)
    {
        switch(t.type)
        {
            case token_type::string:
            {
                printf("string: %s\n",t.literal.c_str());
                break;
            }

            case token_type::integer:
            {
                printf("integer: %s\n",t.literal.c_str());
                break;
            }
        }
    }
}