meta
{
  parse_identifier(p)
  {
    extern _parse_name;
    extern _parser_skip_ws;
    extern _parser_peek;
    extern _parse_call;
    extern _top_level_funcs;
    extern _is_function_name;

    auto name;    
    name = parse_name(p);

    parser_skip_ws(p);
    if (parser_peek(p) == '(') {
        return parse_call(p, name);
    } else {
        auto n;
        if (is_function_name(name, top_level_funcs)) {
            n = make_node(AST_VAR);
            
            n[STRUCT_AST_VAR_NAME] = name;
            n[STRUCT_AST_TYPE] = AST_VAR; // treat as function pointer
            return n;
        }
        n = make_node(AST_VAR);

        n->data.var.name = name;
        return n;
    }
}
  }

  main()
  {
    extern parse_identifier;

    parse_identifier = &my_parse_identifier;
  }
}

main()
{

}