using System;
using System.Collections.Generic;

namespace IniParser.Model;

/// <summary>
///     Information associated to a key from an INI file.
///     Includes both the value and the comments associated to the key.
/// </summary>
public class KeyData : ICloneable
{
	private List<string> _comments;

	private string _value;

	private string _keyName;

	/// <summary>
	/// Gets or sets the comment list associated to this key.
	/// </summary>
	public List<string> Comments
	{
		get
		{
			return _comments;
		}
		set
		{
			_comments = new List<string>(value);
		}
	}

	/// <summary>
	///     Gets or sets the value associated to this key.
	/// </summary>
	public string Value
	{
		get
		{
			return _value;
		}
		set
		{
			_value = value;
		}
	}

	/// <summary>
	///     Gets or sets the name of the key.
	/// </summary>
	public string KeyName
	{
		get
		{
			return _keyName;
		}
		set
		{
			if (value != string.Empty)
			{
				_keyName = value;
			}
		}
	}

	/// <summary>
	///     Initializes a new instance of the <see cref="T:IniParser.Model.KeyData" /> class.
	/// </summary>
	public KeyData(string keyName)
	{
		if (string.IsNullOrEmpty(keyName))
		{
			throw new ArgumentException("key name can not be empty");
		}
		_comments = new List<string>();
		_value = string.Empty;
		_keyName = keyName;
	}

	/// <summary>
	///     Initializes a new instance of the <see cref="T:IniParser.Model.KeyData" /> class
	///     from a previous instance of <see cref="T:IniParser.Model.KeyData" />.
	/// </summary>
	/// <remarks>
	///     Data is deeply copied
	/// </remarks>
	/// <param name="ori">
	///     The instance of the <see cref="T:IniParser.Model.KeyData" /> class 
	///     used to create the new instance.
	/// </param>
	public KeyData(KeyData ori)
	{
		_value = ori._value;
		_keyName = ori._keyName;
		_comments = new List<string>(ori._comments);
	}

	/// <summary>
	///     Creates a new object that is a copy of the current instance.
	/// </summary>
	/// <returns>
	///     A new object that is a copy of this instance.
	/// </returns>
	public object Clone()
	{
		return new KeyData(this);
	}
}
