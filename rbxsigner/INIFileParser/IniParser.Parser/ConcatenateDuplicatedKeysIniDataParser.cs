using IniParser.Model;
using IniParser.Model.Configuration;

namespace IniParser.Parser;

public class ConcatenateDuplicatedKeysIniDataParser : IniDataParser
{
	public new ConcatenateDuplicatedKeysIniParserConfiguration Configuration
	{
		get
		{
			return (ConcatenateDuplicatedKeysIniParserConfiguration)base.Configuration;
		}
		set
		{
			base.Configuration = value;
		}
	}

	public ConcatenateDuplicatedKeysIniDataParser()
		: this(new ConcatenateDuplicatedKeysIniParserConfiguration())
	{
	}

	public ConcatenateDuplicatedKeysIniDataParser(ConcatenateDuplicatedKeysIniParserConfiguration parserConfiguration)
		: base(parserConfiguration)
	{
	}

	protected override void HandleDuplicatedKeyInCollection(string key, string value, KeyDataCollection keyDataCollection, string sectionName)
	{
		keyDataCollection[key] = keyDataCollection[key] + Configuration.ConcatenateSeparator + value;
	}
}
