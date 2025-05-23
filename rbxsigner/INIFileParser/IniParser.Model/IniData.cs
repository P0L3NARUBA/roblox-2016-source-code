using System;
using IniParser.Model.Configuration;
using IniParser.Model.Formatting;

namespace IniParser.Model;

/// <summary>
///     Represents all data from an INI file
/// </summary>
public class IniData : ICloneable
{
	/// <summary>
	///     Represents all sections from an INI file
	/// </summary>
	private SectionDataCollection _sections;

	/// <summary>
	///     See property <see cref="P:IniParser.Model.IniData.Configuration" /> for more information. 
	/// </summary>
	private IniParserConfiguration _configuration;

	/// <summary>
	///     Configuration used to write an ini file with the proper
	///     delimiter characters and data.
	/// </summary>
	/// <remarks>
	///     If the <see cref="T:IniParser.Model.IniData" /> instance was created by a parser,
	///     this instance is a copy of the <see cref="T:IniParser.Model.Configuration.IniParserConfiguration" /> used
	///     by the parser (i.e. different objects instances)
	///     If this instance is created programatically without using a parser, this
	///     property returns an instance of <see cref="T:IniParser.Model.Configuration.IniParserConfiguration" />
	/// </remarks>
	public IniParserConfiguration Configuration
	{
		get
		{
			if (_configuration == null)
			{
				_configuration = new IniParserConfiguration();
			}
			return _configuration;
		}
		set
		{
			_configuration = value.Clone();
		}
	}

	/// <summary>
	/// 	Global sections. Contains key/value pairs which are not
	/// 	enclosed in any section (i.e. they are defined at the beginning 
	/// 	of the file, before any section.
	/// </summary>
	public KeyDataCollection Global { get; protected set; }

	/// <summary>
	/// Gets the <see cref="T:IniParser.Model.KeyDataCollection" /> instance 
	/// with the specified section name.
	/// </summary>
	public KeyDataCollection this[string sectionName]
	{
		get
		{
			if (!_sections.ContainsSection(sectionName))
			{
				if (!Configuration.AllowCreateSectionsOnFly)
				{
					return null;
				}
				_sections.AddSection(sectionName);
			}
			return _sections[sectionName];
		}
	}

	/// <summary>
	/// Gets or sets all the <see cref="T:IniParser.Model.SectionData" /> 
	/// for this IniData instance.
	/// </summary>
	public SectionDataCollection Sections
	{
		get
		{
			return _sections;
		}
		set
		{
			_sections = value;
		}
	}

	/// <summary>
	///     Used to mark the separation between the section name and the key name 
	///     when using <see cref="M:IniParser.Model.IniData.TryGetKey(System.String,System.String@)" />. 
	/// </summary>
	/// <remarks>
	///     Defaults to '.'.
	/// </remarks>
	public char SectionKeySeparator { get; set; }

	/// <summary>
	///     Initializes an empty IniData instance.
	/// </summary>
	public IniData()
		: this(new SectionDataCollection())
	{
	}

	/// <summary>
	///     Initializes a new IniData instance using a previous
	///     <see cref="T:IniParser.Model.SectionDataCollection" />.
	/// </summary>
	/// <param name="sdc">
	///     <see cref="T:IniParser.Model.SectionDataCollection" /> object containing the
	///     data with the sections of the file
	/// </param>
	public IniData(SectionDataCollection sdc)
	{
		_sections = (SectionDataCollection)sdc.Clone();
		Global = new KeyDataCollection();
		SectionKeySeparator = '.';
	}

	public IniData(IniData ori)
		: this(ori.Sections)
	{
		Global = (KeyDataCollection)ori.Global.Clone();
		Configuration = ori.Configuration.Clone();
	}

	public override string ToString()
	{
		return ToString(new DefaultIniDataFormatter(Configuration));
	}

	public virtual string ToString(IIniDataFormatter formatter)
	{
		return formatter.IniDataToString(this);
	}

	/// <summary>
	///     Creates a new object that is a copy of the current instance.
	/// </summary>
	/// <returns>
	///     A new object that is a copy of this instance.
	/// </returns>
	public object Clone()
	{
		return new IniData(this);
	}

	/// <summary>
	///     Deletes all comments in all sections and key values
	/// </summary>
	public void ClearAllComments()
	{
		Global.ClearComments();
		foreach (SectionData section in Sections)
		{
			section.ClearComments();
		}
	}

	/// <summary>
	///     Merges the other iniData into this one by overwriting existing values.
	///     Comments get appended.
	/// </summary>
	/// <param name="toMergeIniData">
	///     IniData instance to merge into this. 
	///     If it is null this operation does nothing.
	/// </param>
	public void Merge(IniData toMergeIniData)
	{
		if (toMergeIniData != null)
		{
			Global.Merge(toMergeIniData.Global);
			Sections.Merge(toMergeIniData.Sections);
		}
	}

	/// <summary>
	///     Attempts to retrieve a key, using a single string combining section and 
	///     key name.
	/// </summary>
	/// <param name="key">
	///     The section and key name to retrieve, separated by <see cref="!:IniParserConfiguration.SectionKeySeparator" />.
	///
	///     If key contains no separator, it is treated as a key in the <see cref="P:IniParser.Model.IniData.Global" /> section.
	///
	///     Key may contain no more than one separator character.
	/// </param>
	/// <param name="value">
	///     If true is returned, is set to the value retrieved.  Otherwise, is set
	///     to an empty string.
	/// </param>
	/// <returns>
	///     True if key was found, otherwise false.
	/// </returns>
	/// <exception cref="T:System.ArgumentException">
	///     key contained multiple separators.
	/// </exception>
	public bool TryGetKey(string key, out string value)
	{
		value = string.Empty;
		if (string.IsNullOrEmpty(key))
		{
			return false;
		}
		string[] array = key.Split(SectionKeySeparator);
		int num = array.Length - 1;
		if (num > 1)
		{
			throw new ArgumentException("key contains multiple separators", "key");
		}
		if (num == 0)
		{
			if (!Global.ContainsKey(key))
			{
				return false;
			}
			value = Global[key];
			return true;
		}
		string text = array[0];
		key = array[1];
		if (!_sections.ContainsSection(text))
		{
			return false;
		}
		KeyDataCollection keyDataCollection = _sections[text];
		if (!keyDataCollection.ContainsKey(key))
		{
			return false;
		}
		value = keyDataCollection[key];
		return true;
	}

	/// <summary>
	///     Retrieves a key using a single input string combining section and key name.
	/// </summary>
	/// <param name="key">
	///     The section and key name to retrieve, separated by <see cref="!:IniParserConfiguration.SectionKeySeparator" />.
	///
	///     If key contains no separator, it is treated as a key in the <see cref="P:IniParser.Model.IniData.Global" /> section.
	///
	///     Key may contain no more than one separator character.
	/// </param>
	/// <returns>
	///     The key's value if it was found, otherwise null.
	/// </returns>
	/// <exception cref="T:System.ArgumentException">
	///     key contained multiple separators.
	/// </exception>
	public string GetKey(string key)
	{
		if (!TryGetKey(key, out var value))
		{
			return null;
		}
		return value;
	}

	/// <summary>
	///     Merge the sections into this by overwriting this sections.
	/// </summary>
	private void MergeSection(SectionData otherSection)
	{
		if (!Sections.ContainsSection(otherSection.SectionName))
		{
			Sections.AddSection(otherSection.SectionName);
		}
		Sections.GetSectionData(otherSection.SectionName).Merge(otherSection);
	}

	/// <summary>
	///     Merges the given global values into this globals by overwriting existing values.
	/// </summary>
	private void MergeGlobal(KeyDataCollection globals)
	{
		foreach (KeyData global in globals)
		{
			Global[global.KeyName] = global.Value;
		}
	}
}
